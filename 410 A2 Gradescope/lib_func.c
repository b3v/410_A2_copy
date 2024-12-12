#include "tappet.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <time.h>
#include <string.h>
#include <glib.h>

    
//1- 100, puts on buffer
void* observe(void *arg) {
    //printf("Observe... %lu %p\n", pthread_self(), &arg);
    args_t *buffer = (args_t *)arg;
    GHashTable *hash_table = g_hash_table_new(g_str_hash, g_str_equal);
    FILE *file;
    
    char filename[] = "test_file.txt";
    file = fopen(filename, "r");
    
    
    if (file == NULL) {
        fprintf(stderr, "Unable to open file %s\n\n\n\n", filename);
        return NULL;
    }
    
    int i;
    char *str;
    
    ////START READING FROM FILE

    //first - check if in hash, if no add
    //second check if curval = hashval if yes make str "", else change and send
    char line[1024];
    while (fgets(line, sizeof(line), file)) {
    // Remove newline character
    //printf("CUR LINE %s", line);
    //fflush(stdout);
        strtok(line, "\n");
        char *name = strtok(line, "=");
        char *value = strtok(NULL, " ");
        //add to hash table then send if not in hash
        if (g_hash_table_contains(hash_table, name) == FALSE ) {
            g_hash_table_insert(hash_table, strdup(name), strdup(value)); 
            str = malloc((strlen(name) + strlen(value) + 2) * sizeof(char));
                
            strcat(str,name);
            strcat(str, "=");
            strcat(str,value);
            
            printf("OBSERVE SENT %s %li\n", str,(strlen(name) + strlen(value) + 2)) ;
            fflush(stdout);  
            re_route_p(buffer->buffer_out, str);
            free(str);
            
            } 
        else{
            //look up val
        gpointer hash_val;
        if( (hash_val = g_hash_table_lookup(hash_table, name))  != NULL){
            
            int hash_int = atoi((char *)hash_val);
            int value_int = atoi(value);
            if (hash_int == value_int){
                //printf("CUR IS + %s %d %d \n", name, hash_int,value_int );
                //fflush(stdout);
                re_route_p(buffer->buffer_out, NULL);
                continue;
            }else{
               
               
                g_hash_table_replace(hash_table, strdup(name), strdup(value));
                str = malloc((strlen(name) + strlen(value) + 2) * sizeof(char));
                
                strcat(str,name);
                strcat(str, "=");
                strcat(str,value);
                
                printf("OBSERVE SENT %s %li\n", str, (strlen(name) + strlen(value) + 2)) ;
                fflush(stdout);  
                re_route_p(buffer->buffer_out, str);
                free(str);
            }   
    }
        }
}
    //printf("OBSERVE SENT T\n");
    //fflush(stdout);
    str = malloc(( sizeof(char)) + 1);
    strcat(str, "T");
    re_route_p(buffer->buffer_out, "T");
    free(arg);
    free(str);
    fclose(file);
    
    return NULL;
}

//reads from buffer, squares and puts back to next
void* reconstruct(void *arg){
    //printf("Reconstruct... %lu\n", pthread_self());
    //fflush(stdout);
    //usleep(60000);
    args_t *buffer = (args_t *)arg;
    
    
    char *data;
   
    char *name_array[50];
    char *value_array[50];
    
    int total_unique = 0;
    
    char *endname = NULL;
    int name_count = 1;
    int i = 0;

    char *str;
    char *str1;
    int string_bool = 0;
    while(1){
       
        
        //printf("Recons Consume... %lu\n", pthread_self());
        //fflush(stdout);
        data = re_route_c(buffer->buffer_in);
        //usleep(30000);
        
        
        if((data != NULL) && strcmp(data, "T") == 0){
            

            str = malloc(( sizeof(char)) + 1);
            strcat(str, "T");
            re_route_p(buffer->buffer_out, "T");
            free(str);
            free(arg);
            
            //printf("Reconstruct...End\n\n %lu\n", pthread_self());
            //fflush(stdout);
            return NULL;;
        }
            char *name;
            char *value;
            if(data != NULL){
                //printf("DATA RECONSTRUCT %s\n\n\n", data);
                //fflush(stdout);  
                name = strtok(data, "=");
                value = strtok(NULL, " ");
                
                //printf("DATA Name %s\n", name);
                //fflush(stdout);
                //printf("DATA VALU %s\n", value);
                //fflush(stdout);
            }
            
            int j = 0;
            
            if(endname == NULL){
                for(j = 0;j < total_unique;j++){
                    //printf("Array Value %s %s %i\n",name_array[j], name, i);
                    fflush(stdout);
                    if ((name_array[j] != NULL) && (strcmp(name, name_array[j]) == 0)){
                        int save = j;
                        while(name_array[j] != NULL){
                            //printf("New End Name %s", name_array[j]);
                            fflush(stdout);
                            endname = strdup(name_array[j]);
                            j++;
                             
                        }
                        int c;
                        if  (str == NULL){
                            str = malloc(5);
                             if (str == NULL) {
                                printf("Memory allocation failed\n");
                                return NULL;
                            }
                        }
                        
                        for(c = 0; c < total_unique; c++){
                            str = realloc(str, (strlen(str) + strlen(name_array[c]) + strlen(value_array[c]) + 4) * sizeof(char));
                            if (str == NULL) {
                                printf("Memory allocation failed\n");
                                return NULL;
                            }
                            strcat(str, strdup(name_array[c]));
                            strcat(str, strdup("="));
                            strcat(str, strdup(value_array[c]));
                            strcat(str, strdup(", "));
                            printf("First %s\n", str);
                            fflush(stdout);
                        }
                        re_route_p(buffer->buffer_out, str);
                        str[0] = '\0';
                        string_bool = 1;
                        free(name_array[save]);
                        free(value_array[save]);
                        name_array[save] = strdup(name);
                        value_array[save] = strdup(value);
                        break;
                    } 
                }
                if (string_bool != 1){
                   
                    name_array[i] = strdup(name);
                    value_array[i] = strdup(value);
                    total_unique++;
                    i++;
                }
                
                }else{
                      
                     
                    int c;
                    if (name_count < total_unique - 1 ){
                        if(data != NULL && strcmp(name_array[name_count], name) == 0){
                            
                            free(value_array[name_count]);
                            value_array[name_count] = strdup(value);
                            //printf("Updates %s %s\n", name_array[name_count], value_array[name_count] );
                            fflush(stdout);
                        }
                        name_count++;

                    }else{
                      
                        
                        if(data != NULL && strcmp(name_array[name_count], name) == 0){
                            
                            free(value_array[name_count]);
                            value_array[name_count] = strdup(value);
                            //printf("Updates %s %s\n", name_array[name_count]  , value_array[name_count] );
                            fflush(stdout);
                        }
                            if  (str1 == NULL){
                                str1 = malloc(5);
                                if (str1 == NULL) {
                                    printf("Memory allocation failed\n");
                                    return NULL;
                            }
                        }
                        for(c = 0; c < total_unique ; c++){
                            //printf("Array Before %s %s \n", name_array[c], value_array[c]);
                            fflush(stdout);

                        }
                        for(c = 0; c < total_unique ; c++){
                                
                                str1 = realloc(str, (strlen(str1) + (strlen(name_array[c]) + strlen(value_array[c])) + 4) * sizeof(char));
                                //printf("Second %s %i\n", str1, total_unique);
                                fflush(stdout);
                                strcat(str1, strdup(name_array[c]));
                                strcat(str1, strdup("="));
                                strcat(str1, strdup(value_array[c]));
                                strcat(str1, strdup(", "));
                            }
                        re_route_p(buffer->buffer_out, str1);
                        str1[0] = '\0';
                        name_count = 0;

                    }
                }
        
        //when we get here, check if alr in
        
        
        printf("Reconstruct...Prod   %lu\n\n\n\n", pthread_self());
        fflush(stdout);
        
        
       
        
        
    }
    
}
    

void* tapplot(void *arg){
    //usleep(120000);
    args_t *buffer = (args_t *)arg;
    int argn = buffer->arg;
    printf("Tapplot... %i \n", argn);
    fflush(stdout);
    FILE *file;
    
    char filename[] = "output_file.txt";
    file = fopen(filename, "w");
    if (file == NULL) {
        fprintf(stderr, "Unable to open file %s\n\n\n\n", filename);
        return NULL;
    }
    
    
    FILE * gnuplotPipe = popen("./live.py", "w");

    if (!gnuplotPipe) {
        fprintf(stderr, "Error opening Gnuplot pipe\n");
        return NULL;
    }
    
    int num;
    char *data;
    char* str;
    
    int sample_count = 0;
    while(1){
        
        
        data = re_route_c (buffer->buffer_in);
        
        printf("Tapp...data %s \n", data);
        fflush(stdout);
        if (data == NULL){
            continue;
        }
        if(strcmp(data, "T") == 0){
            fprintf(gnuplotPipe, " -1=-1 %d \n", sample_count);
            fflush(gnuplotPipe);
            
            fclose(file);
            free(str);
            free(arg);
            return NULL;
        }
        
        
        
        //printf("NUMBER BEFORE WRITTEN %s \n",data );
        //fflush(stdout);
        char* token;
        token = strtok(data, ",");
        int i ;
        for(i = 0; i < argn - 1; i++){
            
            token = strtok(NULL, ",");
        }
        if(token != NULL){
            fprintf(gnuplotPipe, "%s %d \n", token , sample_count);
            fflush(gnuplotPipe);
            fprintf(file, "%s\n", (token));
            fflush(file);
        }
        sample_count += 1;
        
        printf("Tapp...End %s\n", token);
        fflush(stdout);
    }
        
       
    fflush(file);
}
   


