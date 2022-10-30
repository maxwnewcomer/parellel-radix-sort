#include "sys/stat.h"
#include "stdint.h"
#include "fcntl.h"
#include "sys/mman.h"
#include "stdio.h"
#include "sort.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "pthread.h"


int get_file_size(char* filename) {
    struct stat st;
    stat(filename, &st);
    off_t size = st.st_size;
    return size;
}
int counting_sort(record* start, int size, record** lower) {
    printf("%p\n", start);
    for(int i = 0; i < size; i++) {
        // printf("%i\n", i);
        // printf("%x", lower[i]);s
        // printf("%x", &(start[i]));
        // lower[i] =  &(start[i]);
        // memcpy(lower[i], (&(start[i])), sizeof(record*));
    }
        // printf("%i", lower[i]->key);
    return 0;
}

void* t_run(void* in_ta) {
    // bullshit C setup
    thread_args* ta = (thread_args*) in_ta;
    typedef struct t_radix
    {
        int filled;
        int finished_0;
        int first_1;
        int my_tid;
        record* arr_start;
        record** lower[ta->ARR_SIZE];
    } t_radix;

    t_radix* thread_mem = (t_radix *) ta->thread_mem;
    // 
    t_radix* me = &thread_mem[ta->my_tid];
    printf("%2i: [%p]\n", ta->my_tid, thread_mem[ta->my_tid].arr_start);
    // sort 
    counting_sort(me->arr_start, ta->ARR_SIZE, *thread_mem[ta->my_tid].lower);
    // print
    // printf("%i: [", ta->my_tid);
    // for(int i = 0; i < ta->ARR_SIZE; i++) {
    //     printf("%x,", thread_mem[]);
    // }
    return 0;
}

int p_radix_sort(char* filename) {
    size_t pagesize = getpagesize();
    int filesize = get_file_size(filename);
    // get quick approximates
    int MAX_THREADS = filesize / 770; // can auto adjust this based on file size 
    if (MAX_THREADS > 25) MAX_THREADS = 25; // change back to 10
    int ARR_SIZE = filesize / 100 / MAX_THREADS * 1.4;
    ARR_SIZE = 10; // DELETE ME
    MAX_THREADS = 1;
    // add struct definition (have to add here for dynamic arr size)
    typedef struct t_radix
    {
        int filled;
        int finished_0;
        int first_1;
        int my_tid;
        record* arr_start;
        record** lower[ARR_SIZE];
    } t_radix;

    t_radix *thread_mem = malloc(MAX_THREADS*sizeof(t_radix));
    memset(thread_mem, 0, MAX_THREADS*sizeof(t_radix));
    // memory map records
    struct stat sb;
    int f;
    f = open(filename, O_RDONLY);
    fstat(f, &sb);

    record* mapped_records = (record*) mmap((void*) (pagesize * (1<<20)), filesize, PROT_READ || PROT_WRITE, MAP_SHARED, f, 0);
    if(mapped_records == MAP_FAILED) {
        printf("Failed to map memory :/\n");
        exit(EXIT_FAILURE);
    }

    // init structs
    for(int i = 0; i < MAX_THREADS; i++) {
        thread_mem[i].filled = 0;
        thread_mem[i].finished_0 = 0;
        thread_mem[i].my_tid = i;
        thread_mem[i].arr_start = &(mapped_records[i * ARR_SIZE]);
        memset(thread_mem[i].lower, 0, ARR_SIZE); // maybe unnecessary ?? 
    }
    printf("INIT Diagnostic:\n\tfilesize:   %i\n\tthreads:    %i\n\tARR_SIZE:   %i\n\tthreadsize: %li\n", filesize, MAX_THREADS, ARR_SIZE, sizeof(t_radix));


    
    // alloc threads
    pthread_t *threads = malloc(sizeof(pthread_t)*MAX_THREADS);
    // create threads
    for (int i = 0; i < MAX_THREADS; i++) {
        // run t_run mehtod
        thread_args* ta = malloc(8 + 8 + sizeof(&thread_mem));
        ta->ARR_SIZE = ARR_SIZE;
        ta->my_tid = i;
        ta->thread_mem = thread_mem;
        int r = pthread_create(&threads[i], NULL, &t_run, ta);
        if(r != 0) {
            printf("Issue creating thread %i\n", i);
            exit(1);
        }
    }
    for(int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    free(thread_mem);
    free(threads);
    munmap(mapped_records, filesize);
    return 1;
}