#include "stdatomic.h"


#define EXTRA_SIZE 100
#define NUM_EXTRAS_POS 100
#define MIN_LOWER 10
#define KEY_MASK 0xF0000000


typedef struct t_radix
{
    int filled;
    int count_n[NUM_POS_VALUES];
    int inserted;
    int my_tid; // thread id
    pthread_cond_t finished;
    record* arr_start;
    record* out;
} t_radix;

typedef struct shared_memory
{
    pthread_mutex_t* lock;
    pthread_cond_t* checkable;
    int t_turn;  // thread turn
    int curr_idx;
} shared_memory;

typedef struct shared_count
{
    atomic_int* remaining;
} shared_count;

typedef struct globals
{
    int ARR_SIZE;
    int total_records;
    int empty_idxs;
} globals;

typedef struct thread_args
{
    unsigned int my_tid;
    shared_count* s_count;
    shared_memory* s_memory;
    globals* global;
    t_radix* thread_mem;
} thread_args;



void* t_run(void* thread_args);