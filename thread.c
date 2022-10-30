#include "sort.h"
#include "stdio.h"
#include "stdlib.h"

int counting_sort(record* start, int size, record** lower) {
    for(int i = 0; i < size; i++) {
        lower[i] = &start[i];
    }
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
    } t_radix;
    record** lower = malloc(ta->ARR_SIZE * sizeof(record*));
    if(!lower) {
        printf("failed to allocate lower arr");
        exit(EXIT_FAILURE);
    }
    t_radix* thread_mem = (t_radix *) ta->thread_mem;
    t_radix* me = &thread_mem[ta->my_tid];
    // end of bullshit

    // sort 
    counting_sort(me->arr_start, ta->ARR_SIZE, lower);
    // printf("%i: [\n", ta->my_tid);
    for(int i = 0; i < ta->ARR_SIZE; i++) {
        printf("%i\t%p\t%x\n", ta->my_tid, &lower[i]->key, lower[i]->key);
    }
    // printf("]\n");
    return 0;
}