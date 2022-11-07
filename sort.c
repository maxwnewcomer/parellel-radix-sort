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

int p_radix_sort(char* in, char* out) {
    size_t pagesize = getpagesize();
    int filesize = get_file_size(in);
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
        thread_mem[i].filled = 0;
        thread_mem[i].my_tid = i;
        thread_mem[i].arr_start = mapped_records;
        thread_mem[i].out = mapped_out;
        
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
    // init atomics
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
    global->ARR_SIZE = ARR_SIZE;
    global->total_records = filesize / 100;
    global->empty_idxs = (ARR_SIZE*THREADS) - (filesize / 100);
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
    // make sure things are in mem correctly!

    // for(int i = 0; i < filesize / sizeof(record); i++) {
    //     printf("m\t%p\t%8x\n", &mapped_records[i], mapped_records[i].key);
    // }
    // write back mmap
    
    // if(ftruncate(outfile, filesize)) {
    //     printf("failed to truncate");
    //     exit(EXIT_FAILURE);
    // }
    
    // for(int i = 0; i < global->total_records; i++) {
    //     printf("m:\t%08x\n", mapped_out[i].key);
    // }
    msync(mapped_out, filesize, MS_SYNC);
    munmap(mapped_out, filesize);   
    // free all of our stuff!
    munmap(mapped_records, filesize);
    // free(s_count);
    // free(s_memory);
    // free(global);
    // free(thread_mem);
    // free(threads);'
    close(f);
    close(outfile);
    return 1;
}