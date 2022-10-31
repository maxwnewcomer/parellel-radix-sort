#include "sort.h"
#include "stdio.h"
#include "stdlib.h"

// ABC implementation from 
// https://brilliant.org/wiki/radix-sort/
// sort BITS_AT_ONCE bits at a time
// NUM_POS_VALUES = 2^BITS_AT_ONCE
// sorts keys in memory into lower
int counting_sort(record* start, int size, record** lower, int count_n[], int iteration) {
    //
    // int mask = 0;
    // for(int b = 0; b < BITS_AT_ONCE; b++) {
    //     mask = (1 < ((iteration*BITS_AT_ONCE) + b));
    // }
    key_t mask = (NUM_POS_VALUES-1);
    int C[NUM_POS_VALUES];
    mask = mask << (iteration*BITS_AT_ONCE);

    // count occurences of mask
    for(int i = 0; i<size; i++) {
        // eg. C[0xF & key] += 1
        count_n[(start[i].key & mask) >> (iteration*BITS_AT_ONCE)] += 1;
    }
    // printf("[");
    // for(int j=0; j < NUM_POS_VALUES; j++) {
    //     printf("%i ", count_n[j]);
    // }
    // printf("]\n");


    // C conversion for in-thread sort
    C[0] = count_n[0];
    for(int j = 1; j<NUM_POS_VALUES; j++) {
        C[j] = count_n[j] + C[j-1];
    }

    for(int k = size-1; k>=0; k--) {
        C[(start[k].key & mask) >> (iteration*BITS_AT_ONCE)] -= 1;
        lower[C[(start[k].key & mask) >> (iteration*BITS_AT_ONCE)]] = &start[k];
    }

    // for(int i = 0; i < size; i++) {
    //     lower[i] = &start[i];
    // }
    return 0;
}

void* t_run(void* in_ta) {
    // bullshit C setup
    thread_args* ta = (thread_args*) in_ta;
    typedef struct t_radix
    {
        int filled;
        int count_n[NUM_POS_VALUES];
        int first_1;
        int my_tid;
        record* arr_start;
    } t_radix;
    record** lower = malloc(ta->ARR_SIZE * sizeof(record*));
    if(!lower) {
        printf("failed to allocate lower arr");
        exit(1);
    }
    t_radix* thread_mem = (t_radix *) ta->thread_mem;
    t_radix* me = &thread_mem[ta->my_tid];
    // end of bullshit

    // sort 
    counting_sort(me->arr_start, ta->ARR_SIZE, lower, me->count_n, 3);
    // printf("%i: [\n", ta->my_tid);
    for(int i = 0; i < ta->ARR_SIZE; i++) {
        printf("%i\t%p\t%x\n", ta->my_tid, &lower[i]->key, lower[i]->key);
       
    }
    // printf("]\n");
    return 0;
}