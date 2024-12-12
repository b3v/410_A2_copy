#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <unistd.h> 
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <time.h>
#include <string.h>

#define MAX_PROCS 10
#define MAX_PROGRAM_NAME_LENGTH 10

char *input, *output;
int argn = 1;

typedef struct {
    char role;
    char mode;
    int shm_key;
    int buf_len;
    int shm_id;
    void * base_ptr;
} shm_conn_params;

typedef struct {
    char fname[50];
    shm_conn_params l_buf;
    shm_conn_params r_buf;
} func_meta;

shm_conn_params null_buf = {'x','x',0,0,0,NULL};

int shm_pipe(func_meta ** procs, int num_procs, int is_sync, int buf_len){
    pid_t pid;
    
    char shm_key_str_1[20];
    char shm_key_str_2[20];

    char buf_len_str_1[20];
    char buf_len_str_2[20];

    char mode_1[20];
    char mode_2[20];

    char argn_str[20];

    int  keys[MAX_PROCS + 1];

    // set keys for each buffer that will be shared
    for ( int i = 0; i < num_procs ; i++){
        keys[i] = rand() % 9000 + 1000 + 1;
    }

    for ( int i = 0; i < num_procs ; i++)
    {
        /* Initiate the pipeline */
        pid = fork();
        
        if (pid == 0){              /* working in child function */
            
            /* join strings for exec function call */
            if(is_sync){
                sprintf(mode_1,"%c",'s');
                sprintf(mode_2,"%c",'s');
            } else {
                sprintf(mode_1,"%c",'a');
                sprintf(mode_2,"%c",'a');
            }
            
            if(i == 0){
                sprintf(mode_1,"%c",'x');
            } else if(i == num_procs - 1){
                sprintf(mode_2,"%c",'x');
            } 
            
            sprintf(shm_key_str_1,"%d",keys[i]);
            sprintf(shm_key_str_2,"%d",keys[i+1]);

            sprintf(buf_len_str_1,"%d",buf_len);
            sprintf(buf_len_str_2,"%d",buf_len);

            sprintf(argn_str,"%d",argn);

            char *args[10] = {procs[i]->fname, mode_1, shm_key_str_1, buf_len_str_1,
                                                 mode_2, shm_key_str_2, buf_len_str_2, NULL};

            if(i == 0){
                args[7] = input;
                args[8] = output;
                args[9] = NULL;
            } else if (i == 1) {
                args[7] = output;
                args[8] = argn_str;
                args[9] = NULL;
            } else if (i ==2) {
                args[7] = argn_str;
                args[8] = NULL;
            }

            // for (int j=0; j<10; j++){
            //     printf("%s ",args[j]);
            // }
            // printf("\n");

            /* calling function */
            if(execve(procs[i]->fname,args,NULL) != 0){
                perror("execve");
                return 1;
            }

            return 0;
        
        }

        if (pid > 0)
        {
            usleep(1000);
        }
        
    }

    return 0;
}
  
int main(int argc, char * argv[]) {
    int i, num_proc=0, buf_len = 4, is_sync = 1, n=0;
    func_meta *proccesses[MAX_PROCS];

    for (i = 1; i < argc; i++) {
        if (sscanf(argv[i], "arg%d", &n) == 1) {
            argn = n;
        } else if (strcmp(argv[i], "-p1") == 0 && i + 1 < argc) {
            func_meta p1 = {"./", null_buf, null_buf};
            strcat(p1.fname, argv[i+1]);
            proccesses[0] = &p1;            
            i++;
            num_proc++;

        } else if (strcmp(argv[i], "-p2") == 0 && i + 1 < argc) {
            func_meta p2 = {"./", null_buf, null_buf};
            strcat(p2.fname, argv[i+1]);
            proccesses[1] = &p2;         
            i++;
            num_proc++;

        } else if (strcmp(argv[i], "-p3") == 0 && i + 1 < argc) {
            func_meta p3 = {"./", null_buf, null_buf};
            strcat(p3.fname, argv[i+1]);
            proccesses[2] = &p3;  
            i++;
            num_proc++;

        } else if (strcmp(argv[i], "-p4") == 0 && i + 1 < argc) {
            func_meta p4 = {"./", null_buf, null_buf};
            strcat(p4.fname, argv[i+1]);
            proccesses[3] = &p4;  
            i++;
            num_proc++;

        } else if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
            if (strcmp(argv[i + 1], "sync") == 0)
                is_sync = 1;
            else if (strcmp(argv[i + 1], "async") == 0)
                is_sync = 0;
            i++;

        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            buf_len = atoi(argv[i + 1]);
            i++;
        
        } else if (strcmp(argv[i], "argn") == 0 && i + 1 < argc) {
            buf_len = atoi(argv[i + 1]);
            i++;

        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            exit(0);

        } else if (i == argc - 2) {
            input = argv[i];
        } else if (i == argc - 1) {
            output = argv[i];
        } else {
            printf("Invalid argument: %s\n", argv[i]);
            exit(1);
            
        }
    }

    srand(time(NULL));

    shm_pipe(proccesses,num_proc, is_sync, buf_len);

    return 0;
}