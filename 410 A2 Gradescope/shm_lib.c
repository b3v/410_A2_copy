#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <unistd.h>

#define GET_FLAGS 0666
#define CREATE_FLAGS IPC_CREAT | 0666
#define DELAY 100

typedef enum {bit0=0, bit1=1} bit;

typedef struct {
    char role;
    char mode;
    int shm_key;
    int buf_len;
    int shm_id;
    void * base_ptr;
} shm_conn_params;

typedef struct {
  int time;
  int temp;
} data_t;

typedef struct {
  int time;
  int temp;
  char * fields[10];
  int field_vals[10];
} big_data_t;

int shm_conn(shm_conn_params *param){
    
    /* intializing variables */
    int shm_id, flags = 0;
    void *ptr = NULL;
    size_t shm_size = 0;
    
   
    /*
     *  a - asynchronous
     *  s - synchronous
     *  x - no buffer
     */
    if(param -> mode == 's'){
        
        // synchronous create buffer and space for in and out ints
        shm_size = sizeof(data_t) * (param -> buf_len) + sizeof(int) * 2;

    } else if ( param -> mode == 'a' ) {
        
        // asynchronous create simpsons 4 slot buffer
        shm_size = sizeof(data_t) * 5 + sizeof(bit) * 5;

    } else if ( param -> mode == 'x') {
        printf("no buffer\n");
        return 0;
    } else {
        perror("invalid mode");
        return 1;
    }

    /*
     *  c - consumer
     *  p - producer 
     */
    if(param -> role == 'c'){
        
        // if consumer wait for the shm to be created
        usleep(DELAY);
        flags = GET_FLAGS;

    } else if ( param -> role == 'p' ) {
        
        flags = CREATE_FLAGS;

    } else {

        perror("invalid role");
        return 1;

    }

    shm_id = shmget(param -> shm_key, shm_size, flags);
    if (shm_id == -1){
        perror("shmget");
        return 1;
    }
    // printf("shm_key | %d\tshm_id | %d\n",param -> shm_key, shm_id);

    param -> shm_id = shm_id;

    ptr = shmat(shm_id, NULL, 0);
    if (ptr == (void *)-1){
        perror("shmat");
        return 1;
    }

    param -> base_ptr = ptr;

    return 0;
}

int shm_disconn(shm_conn_params *param){
    
    // if this is the consumer delet the shared memory segment
    if (param -> role == 'c' && param -> mode != 'x'){
        shmdt(param -> base_ptr);
        if (shmctl(param -> shm_id, IPC_RMID, NULL) != 0) {
            perror("shmctl");
            return 1;
        }
    } else if ( param -> role == 'p' && param -> mode != 'x'){
        shmdt(param -> base_ptr);
    } 

    return 0;

}

void shm_sync_write(data_t *buf, int time, int temp, int *write_index, int *read_index, int buf_len){
    
    int wr_temp = *write_index;

    // ensure a buffer space is available
    while((wr_temp + 1) % buf_len == *read_index){
        usleep(100);
    }
    // write into buffer
    buf[*write_index].time = time;
    buf[*write_index].temp = temp;

    *write_index = (wr_temp + 1) % buf_len;
}

void shm_sync_read(data_t *buf, data_t *temp_var, int *write_index, int *read_index, int buf_len){
    // ensure new data is available
    while(*write_index == *read_index){
        usleep(100);
    }
    // read from buffer
    temp_var -> time = buf[*read_index].time;
    temp_var -> temp = buf[*read_index].temp;

    *read_index = ((*read_index) + 1) % buf_len;
}

void shm_async_write (data_t item, data_t **buffer ,bit *latest, bit *reading, bit *slot) {
    bit pair, index;
    pair = !(*reading);
    index = !slot[pair];            // Avoids last written slot in this pair, which
                                    // reader may be reading.
    buffer[pair][index] = item;     // Copy item to data slot.
    slot[pair] = index;             // Indicates latest data within selected pair.
    *latest = pair;                 // Indicates pair containing latest data.
}

void shm_async_read (data_t *ret, data_t **buffer,bit *latest, bit *reading, bit *slot) {
    bit pair, index;
    pair = *latest;
    *reading = pair;
    index = slot[pair];
    ret->time = buffer[pair][index].time;
    ret->temp = buffer[pair][index].temp;
}

int shm_conn_big(shm_conn_params *param){
    
    /* intializing variables */
    int shm_id, flags = 0;
    void *ptr = NULL;
    size_t shm_size = 0;
    
   
    /*
     *  a - asynchronous
     *  s - synchronous
     *  x - no buffer
     */
    if(param -> mode == 's'){
        
        // synchronous create buffer and space for in and out ints
        shm_size = sizeof(big_data_t) * (param -> buf_len) + sizeof(int) * 2;

    } else if ( param -> mode == 'a' ) {
        
        // asynchronous create simpsons 4 slot buffer
        shm_size = sizeof(big_data_t) * 5 + sizeof(bit) * 5;

    } else if ( param -> mode == 'x') {
        printf("no buffer\n");
        return 0;
    } else {
        perror("invalid mode");
        return 1;
    }

    /*
     *  c - consumer
     *  p - producer 
     */
    if(param -> role == 'c'){
        
        // if consumer wait for the shm to be created
        usleep(DELAY);
        flags = GET_FLAGS;

    } else if ( param -> role == 'p' ) {
        
        flags = CREATE_FLAGS;

    } else {

        perror("invalid role");
        return 1;

    }

    shm_id = shmget(param -> shm_key, shm_size, flags);
    if (shm_id == -1){
        perror("shmget");
        return 1;
    }
    // printf("shm_key | %d\tshm_id | %d\n",param -> shm_key, shm_id);

    param -> shm_id = shm_id;

    ptr = shmat(shm_id, NULL, 0);
    if (ptr == (void *)-1){
        perror("shmat");
        return 1;
    }

    param -> base_ptr = ptr;

    return 0;
}

void shm_sync_write_big(big_data_t *buf, big_data_t *item, int *write_index, int *read_index, int buf_len){
    
    int wr_temp = *write_index;

    // ensure a buffer space is available
    while((wr_temp + 1) % buf_len == *read_index){
        usleep(100);
    }
    // write into buffer
    buf[*write_index].time = item->time;
    buf[*write_index].temp = item->temp;
    for (int i=0; i<10 ; i++){    
        strcpy(buf[*write_index].fields[i], item->fields[i]);
        buf[*write_index].field_vals[i] = item->field_vals[i];
    }
    *write_index = (wr_temp + 1) % buf_len;
}

void shm_sync_read_big(big_data_t *buf, big_data_t *temp_var, int *write_index, int *read_index, int buf_len){
    // ensure new data is available
    while(*write_index == *read_index){
        usleep(100);
    }
    // read from buffer
    temp_var -> time = buf[*read_index].time;
    temp_var -> temp = buf[*read_index].temp;
    for (int i=0; i<10 ; i++){
        temp_var->fields[i] = buf[*read_index].fields[i];
        temp_var->field_vals[i] = buf[*read_index].field_vals[i];
    }

    *read_index = ((*read_index) + 1) % buf_len;
}

void shm_async_write_big (big_data_t item, big_data_t **buffer ,bit *latest, bit *reading, bit *slot) {
    bit pair, index;
    pair = !(*reading);
    index = !slot[pair];            // Avoids last written slot in this pair, which
                                    // reader may be reading.
                                    // Copy item to data slot.
    buffer[pair][index].time = item.time;    
    buffer[pair][index].temp = item.temp;
    for (int i=0; i<10; i++){
        buffer[pair][index].fields[i] = item.fields[i];
        buffer[pair][index].field_vals[i] = item.field_vals[i];
    }

    slot[pair] = index;             // Indicates latest data within selected pair.
    *latest = pair;                 // Indicates pair containing latest data.
}

void shm_async_read_big (big_data_t *ret, big_data_t **buffer,bit *latest, bit *reading, bit *slot) {
    bit pair, index;
    pair = *latest;
    *reading = pair;
    index = slot[pair];
    ret->time = buffer[pair][index].time;
    ret->temp = buffer[pair][index].temp;
    for (int i=0; i<10 ; i++){
        ret->fields[i] = buffer[pair][index].fields[i];
        ret->field_vals[i] = buffer[pair][index].field_vals[i];
    }
}