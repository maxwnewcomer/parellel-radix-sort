#include "sort.h"
#include "stdio.h"
#include "stdlib.h"

// ABC implementation from 
// https://brilliant.org/wiki/radix-sort/
// sort BITS_AT_ONCE bits at a time
// NUM_POS_VALUES = 2^BITS_AT_ONCE
// sorts keys in memory into lower
int counting_sort(record* start, int size, record** lower, int count_n[], int iteration) {
    key_t mask = (NUM_POS_VALUES-1);
    int C[NUM_POS_VALUES];
    mask = mask << (iteration*BITS_AT_ONCE);

    // count occurences of mask
    for(int i = 0; i<size; i++) {
        // eg. C[0xF & key] += 1
        count_n[(start[i].key & mask) >> (iteration*BITS_AT_ONCE)] += 1;
    }

    // C conversion for in-thread sort
    C[0] = count_n[0];
    for(int j = 1; j<NUM_POS_VALUES; j++) {
        C[j] = count_n[j] + C[j-1];
    }

    // final sort
    for(int k = size-1; k>=0; k--) {
        C[(start[k].key & mask) >> (iteration*BITS_AT_ONCE)] -= 1;
        lower[C[(start[k].key & mask) >> (iteration*BITS_AT_ONCE)]] = &start[k];
    }
    return 0;
}

int move_to_mem(t_radix* thread_mem, record** lower, int tid, int threads) {
    int cur_idx = 0;
    if(tid == 0) {
        //dump first vals into mem if first process
        while(thread_mem[tid].count_n[0] != 0) {
            thread_mem[tid].arr_start[cur_idx] = *lower[cur_idx];
            cur_idx += 1;
            thread_mem[tid].count_n[0] -= 1;
        }
    }
    // while()
    return 0;
}

void* t_run(void* in_ta) {
    // bullshit C setup
    thread_args* ta = (thread_args*) in_ta;
    record** lower = malloc(ta->ARR_SIZE * sizeof(record*));
    if(!lower) {
        printf("failed to allocate lower arr");
        exit(1);
    }
    t_radix* thread_mem = (t_radix *) ta->thread_mem;
    t_radix* me = &thread_mem[ta->my_tid];

    // sort 
    counting_sort(me->arr_start, ta->ARR_SIZE, lower, me->count_n, 0);
    // move_to_mem(thread_mem, lower, ta->my_tid, ta->threads);

    for(int i = 0; i < ta->ARR_SIZE; i++) {
        printf("%i\t%p\t%x\n", ta->my_tid, &lower[i]->key, lower[i]->key);
    }
    return 0;
}