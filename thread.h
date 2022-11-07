#include "stdatomic.h"
#include "pthread.h"


#define EXTRA_SIZE 1000
#define NUM_EXTRAS_POS 100
#define MIN_LOWER 10
#define KEY_MASK 0xF0000000


typedef struct t_radix
{
    int my_tid; // thread id
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

typedef struct globals
{
    int total_records;
} globals;

typedef struct thread_args
{
    unsigned int my_tid;
    shared_memory* s_memory;
    globals* global;
    t_radix* thread_mem;
} thread_args;



void* t_run(void* thread_args);