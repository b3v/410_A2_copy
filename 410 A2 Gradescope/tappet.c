#include <unistd.h> 
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <time.h>
#include <string.h>
#include <sched.h>
#include <stddef.h>
#include <pthread.h>
#define MAX_PROCS 10
#include "lib_func.h"
# define EMPTY 0xFF
////
int buff_type; 
typedef void *(*ThreadFunction)(void *);

typedef enum {bit0=0, bit1=1} bit;
typedef struct {
    bit latest; 
    bit reading;
    char *buffer[2][2];
    bit *slot;
} async_t;



// Initialize four data slots for items.




typedef struct {
  char *data;			/* Slot data.                            */
  size_t size;			/* Size of data.                         */
  pthread_t id;			/* ID of destination thread.             */
} slot_t;

typedef struct{
  slot_t *slot;
  int count;			/* Number of full slots in buffer.       */
  int in_marker;		/* Next slot to add data to.             */
  int out_marker;		/* Next slot to take data from.          */
  pthread_mutex_t mutex;		/* Mutex for shared buffer.                        */
  pthread_cond_t occupied_slot;         /* Condition when >= 1 full buffer slots.          */
  pthread_cond_t empty_slot;            /* Condition when at least 1 empty mailbox slot.   */    
} buffer_t;
typedef struct {
    void *buffer_in;
    void *buffer_out;
    int arg;
    
} args_t;



//Init
buffer_t **buffer_table;
async_t **buffer;
pthread_t *thread_table;

// move to tappet
void init_async(async_t *b){
    printf("Init Sync\n");
    fflush(stdout);
   
    b->latest=b->reading = 0;
    
    
    for (int i = 1; i >=0 ; i--) {
        for (int j = 1; j >=0; j--) {
            b->buffer[i][j] = malloc(sizeof(char));
            if (b->buffer[i][j] == NULL) {
                // Handle memory allocation failure
                fprintf(stderr, "Memory allocation failed\n");
                exit(1);
            }
        }
    }

    b->slot= malloc(2 * sizeof(bit *));
    
   
    
    printf(" POST init Sync\n");
    fflush(stdout);
}
void init_buffer(buffer_t *b, int size){
    int i;
   
    b->in_marker = b->out_marker = b->count = 0;
    b->slot = malloc((size)*sizeof(slot_t));
    
    for (i = 0; i < size; i++)
        b->slot[i].data = NULL;
        
    pthread_mutex_init (&b->mutex, NULL);
    pthread_cond_init (&b->occupied_slot, NULL);
    pthread_cond_init (&b->empty_slot, NULL);
}
int main(int argc, char* argv[]) {
    // parsing logic
    // pass a list of the functions to be run 
    char *proccesses[50];
    int opt;
  

  
    int cur_program = 0;
    
    int argn = 1;
    
    int size = 1;
    char *test;
    char *out;
    
    while ((opt = getopt(argc, argv, "p:a:b:s:")) != -1) {
        switch (opt) {
            case 'p':
        
                proccesses[cur_program] = optarg;
        
                printf("Searching pathname '%s' ...\n", proccesses[cur_program]);
                cur_program++;
                break;
      
            case 'a':
                
                argn = atoi(optarg);
                printf("Argn value is  %i\n", argn);
                break;
            
            case 'b':
                if (strcmp("sync", optarg) == 0){
                    buff_type = 0;
                }
                else if(strcmp("async", optarg) == 0){
                    buff_type = 1;
                }
                else{
                    perror("Not a buffering option\n");
                    exit(1);
                }
                printf("Buffering Style %i\n", buff_type);
                break;
            
            case 's':
                printf("READ %s \n", optarg);
                fflush(stdout);
                size = atoi(optarg);
                
                printf("Size of buffer is %i\n", size);
                break;
            
            case '?':
                // not a regular option
                perror("Not a valid option\n");
                exit(1);
            }
        }
        printf("Cur Location %s %i \n", argv[0], argc);
        fflush(stdout);
        int index;
        for (index = optind; index < argc;index++){
             
            if (strcmp(argv[index],"test_file" )==0){
                test = argv[index];
            }else if (strcmp(argv[index],"output_file" )==0){
                out = argv[index];
            }
            
            printf("Cur file %s \n", argv[index]);
            fflush(stdout);
        }
        
    
    
    //check which buffer while you use
    int i;
    
    thread_table = malloc((cur_program)*sizeof(pthread_t));
    //initialize the buffer pointers for sync
    if (buff_type == 0){
        buffer_table = malloc((cur_program - 1)*sizeof(buffer_t *));
        for(i = 0; i < cur_program - 1;i++){
            buffer_table[i] = malloc(sizeof( buffer_t));
            if (buffer_table[i] == NULL) {
                fprintf(stderr, "Memory allocation failed\n");
            }
            init_buffer(buffer_table[i], size);
    }
    }else{
        //async
        
        buffer = malloc((cur_program - 1)*sizeof(async_t *));
        
        for(i = 0; i < cur_program - 1;i++){
            buffer[i] =  malloc(sizeof(async_t *));
            //printf("Late Read Sync %i\n", i);
            //fflush(stdout);
            if (buffer[i] == NULL) {
                fprintf(stderr, "Memory allocation failed\n");
            }
            init_async(buffer[i]);
        }
    }
    
   
    
    printf("Before Init Thread %lu\n", pthread_self());
    fflush(stdout);
    
    for (i = 0; i < cur_program; i++){
        
        args_t* cons_args = malloc(sizeof(args_t));
        if (cons_args == NULL) {
            perror("Memory allocation failed");
            exit(1);
        }
        cons_args->buffer_in = NULL;
        cons_args->buffer_out = NULL;
        
        cons_args->arg = argn;
        
        if (buff_type == 0){ 
        if(i == 0){
            cons_args->buffer_out = buffer_table[i];
        }else if(i == (cur_program - 1)){
            cons_args->buffer_in = buffer_table[i - 1];
        }else{
            cons_args->buffer_in = buffer_table[i - 1];
            cons_args-> buffer_out = buffer_table[i];
        }
        }else{
        if(i == 0){
            cons_args->buffer_out = buffer[i];
        }else if(i == (cur_program - 1)){
            cons_args->buffer_in = buffer[i - 1];
        }else{
            cons_args->buffer_in = buffer[i - 1];
            cons_args-> buffer_out = buffer[i];
        }
        }
        
        ThreadFunction fn;
        if (strcmp(proccesses[i], "observe") == 0){
            fn = *observe;
        }
        else if (strcmp(proccesses[i], "reconstruct") == 0){
            fn = *reconstruct;
        }
        else if (strcmp(proccesses[i], "tapplot")== 0) {
            fn = *tapplot;
        }
        
       
        if (pthread_create (&thread_table[i],NULL,
			fn, cons_args ) != 0) {
            perror ("Unable to create thread");
            exit (1);
            }
        
       
    }
    
   
    printf("Before Join\n");
    fflush(stdout);
    
    for (i = 0; i < cur_program; i++) {
        pthread_join (thread_table[i], NULL);
        
    }
    printf("After Thread Creation\n");
    fflush(stdout);
    if(buff_type == 0){

        for (i = 0; i < cur_program - 1; i++) {
            free(buffer_table[i]);
            pthread_mutex_destroy (&buffer_table[i]->mutex);
            pthread_cond_destroy (&buffer_table[i]->occupied_slot);
            pthread_cond_destroy (&buffer_table[i]->empty_slot);
        }
        free(buffer_table);
    }else{
          for (i = 0; i < cur_program - 1; i++) {
            free(buffer[i]);
            }
            free(buffer);
    }
    
    return 0;
}



//4SLOT
////////////////////////

#define DELAY 1E8
uint delay = DELAY;
void bufwrite (async_t *b, char* item) {
    int div = 1;
    bit pair, index;
    pair = !(b->reading);
    index = !(b->slot[pair]); // Avoids last written slot in this pair, which
		       // reader may be reading.
    if (item != NULL){
       
        b->buffer[pair][index] = malloc(strlen(item) + 1);
        memcpy(b->buffer[pair][index], item, strlen(item));
        printf("Buff Write %s\n",  b->buffer[pair][index]);
    }else{
        div = 2;
        b->buffer[pair][index] = NULL;
    }
    fflush(stdout);
     // Copy item to data slot.
    b->slot[pair] = index; // Indicates latest data within selected pair.
    b->latest = pair; // Indicates pair containing latest data.
    for (int j = 0; j < (delay); j++); {
            sched_yield ();	;
    }	     
  
}
char* bufread (async_t *b) {
     for (int j = 0; j < (delay) ; j++); {
            sched_yield ();	;
    }
    bit pair, index;
    pair = b->latest;
    b->reading = pair;
    index = b->slot[pair];
    printf("Buff Read %s\n", b->buffer[pair][index]);
    fflush(stdout);
    if (b->buffer[pair][index] == NULL){
        return NULL;
    }
    char *item;
    item = (char *) malloc (strlen(b->buffer[pair][index]));
    strcat(item, b->buffer[pair][index]);
   
    return (item);
}
////consume and produce
void produce(buffer_t *b, char *item) {
    //lock it to add
    printf("Produce started... %s\n", item);
    fflush(stdout);
    //sem_wait(&mutex_sem);
    
    pthread_mutex_lock (&b->mutex);
    
    while (b->count >= 10){
        pthread_cond_wait (&b->empty_slot, &b->mutex);
    }
    if (item == NULL){
        b->slot[b->in_marker].data = NULL;
        b->slot[b->in_marker].size = (1);
    }else{
        b->slot[b->in_marker].data = (char *) malloc ((strlen(item) + 1));
        b->slot[b->in_marker].size = ((strlen(item) + 1));
        printf("Produce size of new string %s %li\n", item, strlen(item));
        fflush(stdout);
    
    
    //printf("%s: item is:%s", __FUNCTION__, item);
    memcpy (b->slot[b->in_marker].data, item, strlen(item));
    }
    b->in_marker++;
    b->in_marker %= 10;
    b->count++;
    
    
    pthread_cond_broadcast (&b->occupied_slot);
    
    pthread_mutex_unlock (&b->mutex);
    //sem_post(&mutex_sem);
    printf("produce...item: %s\n\n", item);
    fflush(stdout);
    return;
}


char* consume (buffer_t *b) {
    pthread_t self=pthread_self();
    //printf("Consume... %li, buffer %p \n ", self, b);
    fflush(stdout);
    char *item;
    
    //sem_wait(&mutex_sem);
    //printf("Consume Lock %p\n",&b->mutex);
    fflush(stdout);
    pthread_mutex_lock (&b->mutex);
    //printf("Vonsume POSTLock %p\n",&b->mutex);
    fflush(stdout);
    while (b->count <= 0){
        pthread_cond_wait (&b->occupied_slot, &b->mutex);
    }
    printf("Consume Lock %li\n",b->slot[b->out_marker].size);
    fflush(stdout);
    if(b->slot[b->out_marker].data == NULL) {
        item = NULL;
        free (b->slot[b->out_marker].data);
    }else{
        item = (char *) malloc (b->slot[b->out_marker].size);
        memcpy (item, b->slot[b->out_marker].data, b->slot[b->out_marker].size);
        free (b->slot[b->out_marker].data);
    }
    b->out_marker++;
    b->out_marker %= 10;
    b->count--;
    
    pthread_cond_broadcast (&b->empty_slot);
    //printf("Consume UnLock %p\n",&b->mutex);
    fflush(stdout);
    pthread_mutex_unlock (&b->mutex);
    //sem_post(&mutex_sem);
    printf("Finished Consume Call with Item and Id: %s %li \n\n", item, self);
    fflush(stdout);
    return (item);
 
}


//rerouting functinos

void re_route_p(void* b, char* item){
    if (buff_type == 0){
        buffer_t *buffer = (buffer_t *)b;
        produce(buffer, item);
    }else{
        async_t *buffer = (async_t *)b;
        bufwrite(buffer, item);
    }
}
char* re_route_c(void* b){
    if (buff_type == 0){
        buffer_t *buffer = (buffer_t *)b;
        return consume(buffer);
    }else{
        async_t *buffer = (async_t *)b;
        return bufread(buffer);
    }
}