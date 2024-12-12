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
typedef struct {
  char *data;			/* Slot data.                            */
  size_t size;			/* Size of data.                         */
  pthread_t id;			/* ID of destination thread.             */
} slot_t;
//comment
typedef struct{
  slot_t slot[10];
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
typedef enum {bit0=0, bit1=1} bit;
typedef struct {
    bit latest; 
    bit reading;
    char* buffer[2][2];
    bit slot[2];
} async_t;
typedef void *(*ThreadFunction)(void *);

void init_buffer(buffer_t *b, int size);
void init_async(async_t *b);

void produce(buffer_t *b, char *item, size_t size);
char* consume (buffer_t *b);

char* bufread (async_t *b);
void bufwrite (async_t *b, char* item);

void re_route_p(void* b, char* item);
char* re_route_c(void* b);