/*
Trivial solution to the readers-writers problem. In this case, we lock the file
for any reader and any writer. While there are no race conditions, this also defeats
the purpose of having it multithreaded in the first place.
*/

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define BUFSIZE 256

pthread_mutex_t lock;
char* random_sentence_array[5] = {"Barking dogs and screaming toddlers have the unique ability to turn friendly neighbors into cranky enemies.\n",
"Flesh-colored yoga pants were far worse than even he feared.\n",
"Italy is my favorite country; in fact, I plan to spend two weeks there next year.\n",
"The old apple revels in its authority.\n",
"The mysterious diary records the voice.\n"};

// File pointer is shared global resource
FILE* fp = NULL;
int num_lines_written = 0;


void error(char* message){
    perror(message);
    exit(1);
}

void* write_test(){
    int max_write = 10;

    // seed random variable
    srand(time(NULL));
    for(int i=0; i<max_write; i++){
        pthread_mutex_lock(&lock);
        
        int r = rand()%5;
        char buf[BUFSIZE];
        bzero(buf, BUFSIZE);

        strcpy(buf, random_sentence_array[r]);
        // always writes to end of file
        fseek(fp, 0, SEEK_END);
        fputs(buf, fp);
        printf("Wrote: %s", buf);

        num_lines_written += 1;

        pthread_mutex_unlock(&lock);
    }

    pthread_exit(NULL);
    return NULL;
}

void* read_test(){
    int max_read = 10;
    
    srand(time(NULL));
    for(int i=0; i<max_read; i++){
        
        pthread_mutex_lock(&lock);
        fseek(fp, 0, SEEK_SET);

        if(num_lines_written != 0){
            int r = rand()%num_lines_written;
            char line[BUFSIZE];
            bzero(line, BUFSIZE);

            // read random lines
            int counter = 0;
            while(fgets(line, BUFSIZE, fp)){
                if(r == counter){
                    line[strcspn(line, "\n")] = 0;
                    printf("Read: %s\n", line);
                    break;
                }
                counter += 1;
                bzero(line, BUFSIZE);
            }
        }
        pthread_mutex_unlock(&lock);
    }
    pthread_exit(NULL);
    return NULL;

}

// input is in format ./rw_zero_solution [FILENAME] [NUM_READER_THREADS] [NUM_WRITER_THREADS]
int main(int argc, char* argv[]){
    //open file
    fp = fopen(argv[1], "w+");

    if(fp == NULL){
        error("Could not open shared file\n");
    }
    
    //create and start pthreads
    int num_reader_threads = atoi(argv[2]);
    int num_writer_threads = atoi(argv[3]);

    pthread_t pthread_arr_reader[num_reader_threads];
    pthread_t pthread_arr_writer[num_writer_threads];

    for(int i=0; i<num_writer_threads; i++){
        if(pthread_create(&pthread_arr_writer[i], NULL, write_test, NULL) != 0){
            error("Pthread create failed\n");
        }
    }

    for(int i=0; i<num_reader_threads; i++){
        if(pthread_create(&pthread_arr_reader[i], NULL, read_test, NULL) != 0){
            error("Pthread create failed\n");
        }
    }


    //joining back pthreads 
    for(int k=0; k<num_reader_threads; k++){
        if(pthread_join(pthread_arr_reader[k], NULL) !=0){
            error("Pthread join failed\n");
        }
    }

    for(int k=0; k<num_writer_threads; k++){
        if(pthread_join(pthread_arr_writer[k], NULL) !=0){
            error("Pthread join failed\n");
        }
    }

    fclose(fp);
}