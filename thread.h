#include "stdatomic.h"

typedef struct t_radix
{
    int filled;
    int count_n[NUM_POS_VALUES];
    int sorted;
    int inserted;
    int my_tid;
    pthread_cond_t finished;
    record* arr_start;
} t_radix;

typedef struct shared_memory
{
    pthread_mutex_t* lock;
    int c_t_arr;
    int c_t_idx;
} shared_memory;

typedef struct shared_count
{
    atomic_int* remaining;
} shared_count;

typedef struct globals
{
    int ARR_SIZE;
    int THREADS;
} globals;

typedef struct thread_args
{
    int my_tid;
    shared_count* s_count;
    shared_memory* s_memory;
    globals* global;
    t_radix* thread_mem;
} thread_args;



void* t_run(void* thread_args);