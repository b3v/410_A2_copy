#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <unistd.h>
#include "shm_lib.h"

int main(int argc, char *argv[]) {
    
    /******* START INTRO SHM *******/
        // printf("***RECONSTRUCT***\n");
    
        // initiate conn params
        shm_conn_params l_buf_params = {'c','s',0, 10, 0, NULL};
        shm_conn_params r_buf_params = {'p','s',1, 10, 0, NULL};
        shm_conn_params *shm_conns[2] = { &l_buf_params , &r_buf_params };
        
        // initiate variables for synchronous  buffer
        int *write1, *read1, *write2, *read2, argn;
        data_t *buf2 = NULL;
        big_data_t *buf1 = NULL;

        // Initialize four data slots for items.
        bit *latest1, *reading1;  // Control variables
        bit *latest2, *reading2;  // Control variables

        big_data_t **abuf1; 
        data_t **abuf2; 
        
        bit *slot1;
        bit *slot2;

        char *output_file = malloc(sizeof(char) * 20);

        /* get the commandline args */
        if(argc < 6)
            return 1;
            
        shm_conns[0]->mode = *(argv[1]);
        shm_conns[0]->shm_key = atoi(argv[2]);
        shm_conns[0]->buf_len = atoi(argv[3]);

        shm_conns[1]->mode = *(argv[4]);
        shm_conns[1]->shm_key = atoi(argv[5]);
        shm_conns[1]->buf_len = atoi(argv[6]);

        output_file = argv[7];
        argn = atoi(argv[8]);

        // 'x' indicates there is no buffer on this side can be customized to function
        if (shm_conns[0] -> mode != 'x')
            shm_conn_big(shm_conns[0]);
        if (shm_conns[1] -> mode != 'x')
            shm_conn(shm_conns[1]);
        
        // cast pointer to data_t and initiate locking functionality
        if(l_buf_params.mode == 's'){
            buf1 = (big_data_t *) (l_buf_params.base_ptr);
            write1 = (int *) (buf1 + l_buf_params.buf_len);
            read1 = write1 + 1;
        } else if (l_buf_params.mode == 'a'){
            abuf1 = (big_data_t **) &(l_buf_params.base_ptr);
            abuf1[0] = (big_data_t *) (l_buf_params.base_ptr);
            abuf1[1] = (big_data_t *) (abuf1[0] + 2);

            latest1 = (bit *) (abuf1[1] + 2);
            reading1 = (bit *) (latest1 + 1);
            slot1 = (bit *) (reading1 + 1);

            slot1[0] = 0;
            slot1[1] = 0;
        }

        if(r_buf_params.mode == 's'){
            buf2 = (data_t *) (r_buf_params.base_ptr);
            write2 = (int *) (buf2 + r_buf_params.buf_len);
            read2 = write2 + 1;
        } else if (r_buf_params.mode == 'a'){
            abuf2 = (data_t **) &(r_buf_params.base_ptr);
            abuf2[0] = (data_t *) (r_buf_params.base_ptr);
            abuf2[1] = (data_t *) (abuf2[0] + 2);

            data_t empty_temp = {0,0};

            abuf2[0][0] = empty_temp;
            abuf2[0][1] = empty_temp;

            abuf2[1][0] = empty_temp;
            abuf2[1][1] = empty_temp;

            latest2 = (bit *) (abuf2[1] + 2);
            reading2 = (bit *) (latest2 + 1);
            slot2 = (bit *) (reading2 + 1);

            *latest2 = bit0;
            *reading2 = bit0;

            slot2[0] = bit0;
            slot2[1] = bit0;
        }

        // initiate values of read and write position
        if (shm_conns[1]->mode == 's'){
            *write2 = 1;
            *read2 = 0;
        }

    /******** END INTRO SHM ********/
    
    FILE * out_fp = fopen(output_file,"a");

    data_t temp_var = {0,0};
    big_data_t big_temp_var;
    int curr_time = 0;
    int curr_temp = 0;
    int currentVals[10] = {0,0,0,0,0,0,0,0,0,0};
    
    for(int i = 0; i < 10; i++){
        big_temp_var.field_vals[i] = 0;
        big_temp_var.fields[i] = malloc(sizeof(char) * 20);
    }
    big_temp_var.time = 0;
    big_temp_var.temp = 0;

    while (big_temp_var.time != -1)
    {
        // READ FROM PREVIOUS BUFFER AND PLACE IN BIG TEMP VARIABLE
        if (l_buf_params.mode == 'a')
            shm_async_read_big(&big_temp_var, abuf1, latest1, reading1, slot1);
        if (l_buf_params.mode == 's')
            shm_sync_read_big(buf1,&big_temp_var, write1, read1, l_buf_params.buf_len);

        // MAKE SURE DATA IS NEW
        if (big_temp_var.time == curr_time){
            continue;
        } else {
            curr_time = big_temp_var.time;
        }

        // IF THE TEMP IS NULL REPLACE WITH CURRENT TEMP
        if (big_temp_var.temp == -1){
            big_temp_var.temp = curr_temp;
        } else {
            curr_temp = big_temp_var.temp;
        }

        // IF THERE ARE ANY NULL ENTRIES REPLACE THE NULL ENTRIES WITH THE CURRENT VALUE
        for (int i =0; i<10 ; i++){
            if (big_temp_var.field_vals[i] == -1){
                big_temp_var.field_vals[i] = currentVals[i];
            } else {
                currentVals[i] =  big_temp_var.field_vals[i];
            }
        }

        // printf("RECONSTRUCT\t{%d,%d}\n",big_temp_var.time,big_temp_var.temp);
        
        // WRITE TO OUTPUT FILE
        for (int i =0; i<10 ; i++){
            fprintf(out_fp,"%d\t\t",big_temp_var.field_vals[i]);
            fflush(out_fp);
        }
        fprintf(out_fp,"\n");
        fflush(out_fp);
        
        // TRANSLATE DATA INTO OBJECT FOR NEXT BUFFER
        temp_var.time = big_temp_var.time;
        temp_var.temp = big_temp_var.field_vals[argn];

        // Write to the next buffer
        if (r_buf_params.mode == 'a')
            shm_async_write(temp_var, abuf2, latest2, reading2, slot2);
        if (r_buf_params.mode == 's')
            shm_sync_write(buf2, temp_var.time, temp_var.temp, write2, read2, r_buf_params.buf_len);

        usleep(1000);
    }

    // WRITE END CHARACTER
    temp_var.time = -1;
    temp_var.temp = 0;
    if (r_buf_params.mode == 'a')
        shm_async_write(temp_var, abuf2, latest2, reading2, slot2);
    if (r_buf_params.mode == 's')
        shm_sync_write(buf2, -1, 0, write2, read2, r_buf_params.buf_len);
    

    /******* START OUTRO SHM *******/

        shm_disconn(shm_conns[0]);
        shm_disconn(shm_conns[1]);

    /******** END OUTRO SHM ********/

    return 0;
}