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


int get_file_size(char* filename) {
    struct stat st;
    if(stat(filename, &st) == -1) {
        return -1;
    }
    off_t size = st.st_size;
    return size;
}

int p_radix_sort(char* in, char* out) {
    size_t pagesize = getpagesize();
    int filesize = get_file_size(in);
    if(filesize <= 0) {
        fprintf(stderr, "An error has occurred\n");
        exit(0);
    }
    // get quick approximates
    float div_result = filesize / 100.0 / (float)THREADS;
    int ARR_SIZE = (int)div_result;
    if (ARR_SIZE - div_result < 0) ARR_SIZE += 1;

    t_radix* thread_mem = malloc(THREADS*sizeof(t_radix));
    memset(thread_mem, 0, THREADS*sizeof(t_radix));
    // memory map records
    struct stat sb;

    int f = open(in, O_RDONLY);

    fstat(f, &sb);
    record* mapped_records = (record*) mmap((void*) (pagesize * (1<<20)), filesize, PROT_READ, MAP_SHARED, f, 0);
    if(mapped_records == MAP_FAILED) {
        printf("Failed to map memory :/\n");
        exit(1);
    }

    int outfile = open(out, O_RDWR | O_CREAT, 0666);
    // set filesize
    lseek(outfile, filesize-1, SEEK_SET);
    if(write(outfile, "", 1) == -1) {
        printf("couldn't write\n");
    }

    record* mapped_out = mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, outfile, 0);
    if(mapped_out == MAP_FAILED) {
        printf("Failed to map out memory :/\n");
        exit(1);
    }
    // init structs
    for(int i = 0; i < THREADS; i++) {
        thread_mem[i].my_tid = i;
        thread_mem[i].arr_start = mapped_records;
        thread_mem[i].out = mapped_out;      
    }
    // printf("INIT Diagnostic:\n\tfilesize:   %i\n\tthreads:    %i\n\tARR_SIZE:   %i\n\tmem/thread: %li\n\n", filesize, THREADS, ARR_SIZE, sizeof(t_radix));
    // alloc shared_memory
    shared_memory* s_memory = malloc(sizeof(shared_memory));
    if(!s_memory) {
        printf("failed to alloc memory_lock");
        exit(EXIT_FAILURE);
    }
    s_memory->lock = malloc(sizeof(pthread_mutex_t));
    if(!s_memory->lock) {
        printf("failed to allocated lock");
        exit(EXIT_FAILURE);
    }
    s_memory->checkable = malloc(sizeof(pthread_cond_t));
    if(!s_memory->checkable) {
        printf("failed to allocated lock");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_init(s_memory->lock, NULL);
    pthread_cond_init(s_memory->checkable, NULL);
    s_memory->curr_idx = 0;
    s_memory->t_turn = 0;
    // alloc globals
    globals* global = malloc(sizeof(globals));
    if(!global) {
        printf("failed to alloc globals");
        exit(EXIT_FAILURE);
    }
    global->total_records = filesize / 100;
    // alloc threads
    pthread_t *threads = malloc(sizeof(pthread_t)*THREADS);
    // create threads

    for (int i = 0; i < THREADS; i++) {
        // run t_run mehtod
        thread_args* ta = malloc(sizeof(thread_args));
        if(!ta) {
            printf("failed to allocate ta");
            exit(EXIT_FAILURE);
        }
        ta->thread_mem = thread_mem;
        ta->s_memory = s_memory;
        ta->global = global;
        ta->my_tid = i;
        int r = pthread_create(&threads[i], NULL, &t_run, ta);
        if(r != 0) {
            printf("Issue creating thread [%i]\n", i);
            exit(1);
        }
    }

    for(int i = 0; i < THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    // make sure things are in mem correctly!

    // for(int i = 0; i < global->total_records; i++) {
    //     printf("m:\t%i\n", mapped_out[i].key);
    // }
    msync(mapped_out, filesize, MS_ASYNC);
       // free all of our stuff!
    munmap(mapped_out, filesize);   
    munmap(mapped_records, filesize);
    free(s_memory);
    free(global);
    free(thread_mem);
    free(threads);
    close(f);
    close(outfile);
    return 1;
}