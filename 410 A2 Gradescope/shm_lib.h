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
  char * fields[10];
  int field_vals[10];
} big_data_t;

typedef struct {
  int time;
  int temp;
} data_t;

int shm_conn(shm_conn_params *param);

int shm_disconn(shm_conn_params *param);

void shm_sync_write(data_t *buf, int time, int temp, int *write_index, int *read_index, int buf_len);

void shm_sync_read(data_t *buf, data_t *temp_var, int *write_index, int *read_index, int buf_len);

void shm_async_write (data_t item, data_t **buffer ,bit *latest, bit *reading, bit *slot) ;

void shm_async_read (data_t *ret, data_t **buffer,bit *latest, bit *reading, bit *slot) ;

int shm_conn_big(shm_conn_params *param);

void shm_sync_write_big(big_data_t *buf, big_data_t *item, int *write_index, int *read_index, int buf_len);

void shm_sync_read_big(big_data_t *buf, big_data_t *temp_var, int *write_index, int *read_index, int buf_len);

void shm_async_write_big (big_data_t item, big_data_t **buffer ,bit *latest, bit *reading, bit *slot);

void shm_async_read_big (big_data_t *ret, big_data_t **buffer,bit *latest, bit *reading, bit *slot);