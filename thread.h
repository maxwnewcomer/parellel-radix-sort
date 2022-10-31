typedef struct t_radix
{
    int filled;
    int count_n[NUM_POS_VALUES];
    int last_idx;
    int my_tid;
    record* arr_start;
} t_radix;

typedef struct thread_args
{
    int ARR_SIZE;
    int filesize;
    int my_tid;
    int threads;
    void* thread_mem;
} thread_args;

void* t_run(void* thread_args);