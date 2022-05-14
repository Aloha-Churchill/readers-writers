#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <semaphore.h>

#define BUFSIZE 256

sem_t resource_lock;
sem_t reader_lock;
int reader_count = 0;

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

void* write_test(void* fp_void){
    int max_write = 10;

    // seed random variable
    srand(time(NULL));
    for(int i=0; i<max_write; i++){
        // lock shared resource for writing
        sem_wait(&resource_lock);
        
        int r = rand()%5;
        char buf[BUFSIZE];
        bzero(buf, BUFSIZE);

        strcpy(buf, random_sentence_array[r]);
        // always writes to end of file
        fseek(fp, 0, SEEK_END);
        fputs(buf, fp);
        printf("Wrote: %s", buf);

        num_lines_written += 1;
        // unlock shared resource from writing
        sem_post(&resource_lock);
    }


    pthread_exit(NULL);
    return NULL;
}

void* read_test(void* fp_void){
    //each reader has own copy of file pointer
    FILE* fp_unique = (FILE*)fp_void;
    int max_read = 10;

    // critical section for decrementing reader_count, so lock reader_lock
    sem_wait(&reader_lock);
    reader_count += 1;

    // if first reader, then lock resource for reading only
    if(reader_count == 1){
        sem_wait(&resource_lock);
    }

    // unlock reader semaphore for other readers
    sem_post(&reader_lock);
    
    // read from file
    srand(time(NULL));
    for(int i=0; i<max_read; i++){
        fseek(fp_unique, 0, SEEK_SET);
        if(num_lines_written != 0){
            int r = rand()%num_lines_written;
            char line[BUFSIZE];
            bzero(line, BUFSIZE);
            // read random lines
            int counter = 0;
            while(fgets(line, BUFSIZE, fp_unique)){
                if(r == counter){
                    line[strcspn(line, "\n")] = 0;
                    printf("Read: %s\n", line);
                    break;
                }
                counter += 1;
                bzero(line, BUFSIZE);
            }
        }
    }

    // critical section for decrementing reader_count, so lock reader_lock
    sem_wait(&reader_lock);
    reader_count -= 1;

    // if no more readers, then release file from read mode
    if(reader_count == 0){
        sem_post(&resource_lock);
    }

    // let other readers enter     
    sem_post(&reader_lock);   
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

    //initialize semaphores
    sem_init(&resource_lock, 0, 1);
    sem_init(&reader_lock, 0, 1);

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
        if(pthread_create(&pthread_arr_reader[i], NULL, read_test, fp) != 0){
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

    //destroy semaphores
    sem_destroy(&resource_lock);
    sem_destroy(&reader_lock);

    fclose(fp);
}