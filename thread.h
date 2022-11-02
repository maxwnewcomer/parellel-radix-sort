#include "stdatomic.h"

typedef struct t_radix
{
    int filled;
    int count_n[NUM_POS_VALUES];
    int inserted;
    int my_tid; // thread id
    pthread_cond_t finished;
    record* arr_start;
} t_radix;

typedef struct shared_memory
{
    pthread_mutex_t* lock;
    int t_turn;  // thread turn
    int c_t_arr; // current thread arr
    int c_t_idx; // current thread arr index
} shared_memory;

typedef struct shared_count
{
    atomic_int* remaining;
} shared_count;

typedef struct globals
{
    int ARR_SIZE;
    int THREADS;
    int total_records;
    int empty_idxs;
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