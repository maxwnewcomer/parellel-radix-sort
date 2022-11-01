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
#include "stdatomic.h"


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
        
        pthread_cond_init(&thread_mem[i].finished, NULL);
        
    }
    printf("INIT Diagnostic:\n\tfilesize:   %i\n\tthreads:    %i\n\tARR_SIZE:   %i\n\tmem/thread: %li\n\n", filesize, THREADS, ARR_SIZE, sizeof(t_radix));

    // alloc shared_count
    shared_count* s_count = malloc(sizeof(s_count));
    if(!s_count) {
        printf("failed to alloc big_count");
        exit(EXIT_FAILURE);
    }

    s_count->remaining = (atomic_int *) malloc(NUM_POS_VALUES * sizeof(atomic_int));
    if(!s_count->remaining) {
        printf("failed to alloc remaining count");
        exit(EXIT_FAILURE);
    }
    for(int i = 0; i <  NUM_POS_VALUES; i++) {
        atomic_init(&s_count->remaining[i], 0);
    }
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
    pthread_mutex_init(s_memory->lock, NULL);
    s_memory->c_t_arr = 0;
    s_memory->c_t_idx = 0;
    // alloc globals
    globals* global = malloc(sizeof(globals));
    if(!global) {
        printf("failed to alloc globals");
        exit(EXIT_FAILURE);
    }
    global->ARR_SIZE = ARR_SIZE;
    global->THREADS = THREADS;
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
        ta->s_count = s_count;
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

    for(int i = 0; i < filesize / sizeof(record); i++) {
        printf("m\t%p\t%8x\n", &mapped_records[i], mapped_records[i].key);
    }
    msync(mapped_records, filesize, 0);
    munmap(mapped_records, filesize);
    free(s_count);
    free(s_memory);
    free(global);
    free(thread_mem);
    free(threads);
    return 1;
}