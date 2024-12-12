#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <unistd.h>
#include "shm_lib.h"

int is_member(char **types, char *type){
    for(int i=0; i< 10; i++){
        // printf("checking types[%d]\n",i);
        if (strcmp(type,types[i]) == 0)
            return 1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    
    /******* START INTRO SHM *******/
        // printf("***OBSEREVE***\n");
    
        // initiate conn params
        shm_conn_params l_buf_params = {'c','s',0, 10, 0, NULL};
        shm_conn_params r_buf_params = {'p','s',1, 10, 0, NULL};
        shm_conn_params *shm_conns[2] = { &l_buf_params , &r_buf_params };
        
        // initiate variables for synchronous  buffer
        int *write1, *read1, *write2, *read2;
        big_data_t *buf1 = NULL, *buf2 = NULL;

        // Initialize four data slots for items.
        bit *latest1, *reading1;  // Control variables
        bit *latest2, *reading2;  // Control variables

        big_data_t **abuf1; 
        big_data_t **abuf2; 
        
        bit *slot1;
        bit *slot2;

        char *input_file, *output_file;

        /* get the commandline args */
        if(argc < 6)
            return 1;
            
        shm_conns[0]->mode = *(argv[1]);
        shm_conns[0]->shm_key = atoi(argv[2]);
        shm_conns[0]->buf_len = atoi(argv[3]);

        shm_conns[1]->mode = *(argv[4]);
        shm_conns[1]->shm_key = atoi(argv[5]);
        shm_conns[1]->buf_len = atoi(argv[6]);

        input_file = argv[7];
        output_file = argv[8];

        // 'x' indicates there is no buffer on this side can be customized to function
        if (shm_conns[0] -> mode != 'x')
            shm_conn_big(shm_conns[0]);
        if (shm_conns[1] -> mode != 'x')
            shm_conn_big(shm_conns[1]);
        
        // cast pointer to big_data_t and initiate locking functionality
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
            buf2 = (big_data_t *) (r_buf_params.base_ptr);

            for(int i = 0; i < r_buf_params.buf_len; i++){
                for (int j=0; j < 10; j++){
                    buf2[i].field_vals[j] = 0;
                    buf2[i].fields[j] = malloc(sizeof(char) * 20);
                }
            }

            write2 = (int *) (buf2 + r_buf_params.buf_len);
            read2 = write2 + 1;
        } else if (r_buf_params.mode == 'a'){
            abuf2 = (big_data_t **) &(r_buf_params.base_ptr);
            abuf2[0] = (big_data_t *) (r_buf_params.base_ptr);
            abuf2[1] = (big_data_t *) (abuf2[0] + 2);

            big_data_t empty_temp;

            for(int i = 0; i < 10; i++){
                empty_temp.field_vals[i] = 0;
                empty_temp.fields[i] = malloc(sizeof(char) * 20);
            }
            empty_temp.time = 0;
            empty_temp.temp = 0;

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
    
    char line[100];
    int currentTemp = 0;
    int currentVals[20];

    char *types[10];
    char type[20];

    int num_types=0;

    big_data_t big_temp_var;

    for(int i=0; i<10; i++){
        types[i] = malloc(sizeof(char) * 20);
        big_temp_var.fields[i] = malloc(sizeof(char) * 20);
        big_temp_var.field_vals[i] = 0;
    }

    FILE * test_in, *out_fp;
    test_in = fopen(input_file,"r");
    out_fp = fopen(output_file,"w");

    // ESTABLISH DIFFERENT TYPES
    while (fgets(line, sizeof(line), test_in) != NULL){
        int value;
        if (sscanf(line, "%[^=]=%d", type, &value) == 2){
            if (num_types == 0)
            {
                strcpy(types[0],type);
                if (strcmp(type, "temperature") == 0)
                    currentTemp = value;
                
                currentVals[0] = value;
                big_temp_var.field_vals[0] = value;
                num_types++;
            } else if (is_member(types,type) == 0) {
                strcpy(types[num_types], type);
                if (strcmp(type, "temperature") == 0)
                    currentTemp = value;

                currentVals[num_types] = value;
                big_temp_var.field_vals[num_types] = value;
                num_types++;
            } else {
                break;
            }            
        }
    }

    for(int i =0; i < num_types; i++){
        strcpy(big_temp_var.fields[i], types[i]);
        fprintf(out_fp,"%.4s\t",types[i]);
        fflush(out_fp);
    }
    fprintf(out_fp,"\n");
    fflush(out_fp);



    // WRITE OBSERVATIONS TO NEXT BUFFER
    while (fgets(line, sizeof(line), test_in) != NULL) {
        char type[20];
        int value;
                
        if (sscanf(line, "%[^=]=%d", type, &value) == 2) {
            for (int i=0; i <num_types; i++){
                if (strcmp(type, types[i]) == 0){
                    currentVals[i] = value;
                }
            }
            if (strcmp(type, "temperature") == 0) {
                currentTemp = value;
            } else if (strcmp(type, "time") == 0) {
                big_temp_var.time = value;
                big_temp_var.temp = currentTemp;

                for (int i=0; i <num_types; i++){
                    big_temp_var.field_vals[i] = currentVals[i];
                }

                // printf("OBSERVE\t{%d,%d}\n",value,currentTemp);

                if (r_buf_params.mode == 'a')
                    shm_async_write_big(big_temp_var, abuf2, latest2, reading2, slot2);
                if (r_buf_params.mode == 's')
                    shm_sync_write_big(buf2, &big_temp_var, write2, read2, r_buf_params.buf_len);

                currentTemp = -1;

                usleep(10000);
                
            } 
        }
    }

    // WRITE END CHARACTER
    big_temp_var.time = -1;
    big_temp_var.temp = 0;
    if (r_buf_params.mode == 'a')
        shm_async_write_big(big_temp_var, abuf2, latest2, reading2, slot2);
    if (r_buf_params.mode == 's')
        shm_sync_write_big(buf2, &big_temp_var, write2, read2, r_buf_params.buf_len);

    /******* START OUTRO SHM *******/

        fclose(test_in);
        fclose(out_fp);
        shm_disconn(shm_conns[0]);
        shm_disconn(shm_conns[1]);

    /******** END OUTRO SHM ********/

    return 0;
}