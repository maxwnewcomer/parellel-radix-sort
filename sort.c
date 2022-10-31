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
#include "thread.h"
#include "math.h"


int get_file_size(char* filename) {
    struct stat st;
    stat(filename, &st);
    off_t size = st.st_size;
    return size;
}

int p_radix_sort(char* filename) {
    size_t pagesize = getpagesize();
    int filesize = get_file_size(filename);
    // get quick approximates
    int THREADS = filesize / 1000; // can auto adjust this based on file size 
    if (THREADS < 1) THREADS = 1;
    if (THREADS > MAX_THREADS) THREADS = MAX_THREADS; // change back to 10
    float div_result = filesize / 100.0 / (float)THREADS;
    int ARR_SIZE = (int)div_result;
    if (ARR_SIZE - div_result < 0) ARR_SIZE += 1;

    t_radix *thread_mem = malloc(THREADS*sizeof(t_radix));
    memset(thread_mem, 0, THREADS*sizeof(t_radix));
    // memory map records
    struct stat sb;
    int f;
    f = open(filename, O_RDWR);
    fstat(f, &sb);

    record* mapped_records = (record*) mmap((void*) (pagesize * (1<<20)), filesize, PROT_READ | PROT_WRITE, MAP_SHARED, f, 0);
    if(mapped_records == MAP_FAILED) {
        printf("Failed to map memory :/\n");
        exit(1);
    }
    // init structs
    for(int i = 0; i < THREADS; i++) {
        thread_mem[i].filled = 0;
        thread_mem[i].my_tid = i;
        thread_mem[i].arr_start = &(mapped_records[i * ARR_SIZE]);
        thread_mem[i].last_idx = 0;
    }
    printf("INIT Diagnostic:\n\tfilesize:   %i\n\tthreads:    %i\n\tARR_SIZE:   %i\n\tmem/thread: %li\n\n", filesize, THREADS, ARR_SIZE, sizeof(t_radix));

    // alloc threads
    pthread_t *threads = malloc(sizeof(pthread_t)*THREADS);
    // create threads
    for (int i = 0; i < THREADS; i++) {
        // run t_run mehtod
        thread_args* ta = malloc(8 + 8 + 8 + 8 + sizeof(&thread_mem));
        ta->ARR_SIZE = ARR_SIZE;
        ta->filesize = filesize;
        ta->my_tid = i;
        ta->threads = THREADS;
        ta->thread_mem = thread_mem;
        int r = pthread_create(&threads[i], NULL, &t_run, ta);
        if(r != 0) {
            printf("Issue creating thread [%i]\n", i);
            exit(1);
        }
    }
    for(int i = 0; i < THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    msync(mapped_records, filesize, 0);
    munmap(mapped_records, filesize);
    free(thread_mem);
    free(threads);
    return 1;
}