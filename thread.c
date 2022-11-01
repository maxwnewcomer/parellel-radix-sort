#include "sort.h"
#include "stdio.h"
#include "stdlib.h"
#include "thread.h"
#include "string.h"
#include "sys/mman.h"
#include "unistd.h"
#include "pthread.h"
#include "stdatomic.h"


// ABC implementation from
// https://brilliant.org/wiki/radix-sort/
// 
// All shifting and masking original, array layouts from brilliant
// count_n = C 
// lower = B (kind of, B doesn't hold the values, only pointers to the values)
// start = A
// sort BITS_AT_ONCE bits at a time
// NUM_POS_VALUES = 2^BITS_AT_ONCE
// 
// 
// assume iteration = 1, BITS AT_ONCE = 4
// start[k].key                                    = 0x1A93CFD2
// mask << (iteration*BITS_AT_ONCE)                = 0x000000F0
// start[k].key & mask                             = 0x000000D0
// start[k].key & mask >> (iteration*BITS_AT_ONCE) = 0x0000000D
//                                         indexable   ^^^^^^^^
// 
// sorts keys in memory into lower
int counting_sort(record* start, int size, record* lower, int* count_n, shared_count* s_count, int iteration) {
    key_t mask = (NUM_POS_VALUES-1);
    int C[NUM_POS_VALUES];
    mask = mask << (iteration*BITS_AT_ONCE);
    // printf("yo\n");

    // count occurences of mask
    for(int i = 0; i<size; i++) {
        // eg. C[0xF & key] += 1
        count_n[(unsigned)(start[i].key & mask) >> (iteration*BITS_AT_ONCE)] += 1;
        // IMPORTANT TO USE UNSIGNED
    }
    // get sum-up_to[j]
    for(int j = 1; j<NUM_POS_VALUES; j++) {
        count_n[j] = count_n[j] + count_n[j-1];
        C[j] = count_n[j];
    }

    // final sort
    for(int k = size-1; k>=0; k--) {
        C[(unsigned)(start[k].key & mask) >> (iteration*BITS_AT_ONCE)] -= 1;
        lower[C[(unsigned)(start[k].key & mask) >> (iteration*BITS_AT_ONCE)]] = start[k];
    }

    for(int i = 0; i < NUM_POS_VALUES; i++) {
        atomic_fetch_add_explicit(&s_count->remaining[i], count_n[i], memory_order_acq_rel);
    }
    return 0;
}
int move_to_mem(t_radix* thread_mem, record* lower, thread_args* ta) {
    // t_radix* me = &thread_mem[ta->my_tid];
    // shared_memory* s_mem = ta->s_memory;
    // shared_count* s_count = ta->s_count;
    // while(s_count[NUM_POS_VALUES-1] != )


    return 0;
}

void* t_run(void* in_ta) {
    // bullshit C setup
    thread_args* ta = (thread_args*) in_ta;

    // extract ta
    // printf("hello\n");

    globals* global = ta->global;
    shared_count* s_count = ta->s_count;
    t_radix* thread_mem = ta->thread_mem;

    // setup lower
    record* lower = malloc(global->ARR_SIZE * sizeof(record));
    if(!lower) {
        printf("failed to alloc lower");
        exit(EXIT_FAILURE);
    }
    
    t_radix* me = &thread_mem[ta->my_tid];
    for(int r = 0; r < 1; r++) {
       
        me->sorted = 0;
        counting_sort(me->arr_start, global->ARR_SIZE, lower, me->count_n, s_count, r);
        // me->sorted = 1;
        // for(int i = 0; i < global->ARR_SIZE; i++) {
        //     printf("%08x\n", lower[i].key);
        // }
        // for(int i = 0; i < NUM_POS_VALUES; i++) {
        //     printf("%3i ", me->count_n[i]);
        // }
        // printf("\n");
        // for(int i = 0; i < NUM_POS_VALUES; i++) {
        //     printf("%3i ", s_count->remaining[i]);
        // }
        // printf("\n");
        // move_to_mem(thread_mem, lower, ta);        
    }
    free(lower);
    return 0;

}