#include "sort.h"
#include "stdio.h"
#include "stdlib.h"
#include "thread.h"
#include "string.h"
#include "sys/mman.h"


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
int counting_sort(record* start, int size, void* in_lower, int count_n[], int iteration) {
    // quirky c :()
    typedef struct t_lower
    {
        record records[size];
        record* old_pointers[size];
    } t_lower;
    t_lower* lower = (t_lower*) in_lower;

    key_t mask = (NUM_POS_VALUES-1);
    int C[NUM_POS_VALUES];
    mask = mask << (iteration*BITS_AT_ONCE);
    // printf("\nmask:                 0x%08x\n", mask);
    // count occurences of mask
    for(int i = 0; i<size; i++) {
        // eg. C[0xF & key] += 1
        count_n[(unsigned)(start[i].key & mask) >> (iteration*BITS_AT_ONCE)] += 1;
        // IMPORTANT TO USE UNSIGNED
    }
    // C conversion for in-thread sort
    C[0] = count_n[0];
    for(int j = 1; j<NUM_POS_VALUES; j++) {
        C[j] = count_n[j] + C[j-1];
    }

    // final sort
    for(int k = size-1; k>=0; k--) {
        C[(unsigned)(start[k].key & mask) >> (iteration*BITS_AT_ONCE)] -= 1;
        lower->records[C[(unsigned)(start[k].key & mask) >> (iteration*BITS_AT_ONCE)]] = start[k];
        lower->old_pointers[C[(unsigned)(start[k].key & mask) >> (iteration*BITS_AT_ONCE)]] = &start[k];
    }
    return 0;
}



int move_to_mem(t_radix* thread_mem, void* in_lower, thread_args* ta) {
    typedef struct t_lower
    {
        record records[ta->ARR_SIZE];
        record* old_pointers[ta->ARR_SIZE];
    } t_lower;
    t_lower* lower = (t_lower *) in_lower;
    int one_inited = 0;
    int count_idx = 0;
    int lower_idx = 0;
    int tid = ta->my_tid;
    t_radix* me = &thread_mem[ta->my_tid];
    t_radix* before_me;
    if (tid == 0) {
        before_me = &thread_mem[ta->threads - 1];
    } else {
        before_me = &thread_mem[ta->my_tid - 1];
    }
    me->filled = 0;
    // printf("\ntid: %i\n", tid);
    printf("count_c:          [");
    for(int x = 0; x < NUM_POS_VALUES; x++) {
        printf("%3i ", thread_mem[tid].count_n[x]);
    }
    printf("]\n");
    // printf("]\nlower_records:    [");
    // for(int x = 0; x < ta->ARR_SIZE; x++) {
    //     printf("%8x  ",  lower->records[x].key);
    // }
    // printf("]\nactual_mem:       [");
    // for(int x = 0; x < ta->ARR_SIZE; x++) {
    //     printf("%8x  ", thread_mem[tid].arr_start[x].key);
    // }
    // printf("]\n");
    while(count_idx < NUM_POS_VALUES) {
        
        // wait until ready
        if(tid == 0) {
            
            if(count_idx == 0) {;;}
            else {
                // printf("TID:%2i\n\tWaiting On:          %2i for %2i\n\tSo I Can Insert:   %2i \n", ta->my_tid, ta->threads - 1, count_idx-1, count_idx);
                while(!before_me->sorted || before_me->count_n[count_idx-1] != 0) {}
            }
            if(!one_inited) {
                *ta->add_thread = 0;
                *ta->add_idx = 0;
                one_inited++;
            }
            
        } else  {
            // printf("TID:%2i\n\tWaiting On:         %2i for %2i\n\tSo I can Insert:   %2i \n\n", ta->my_tid, ta->my_tid - 1,count_idx, count_idx);
            while(before_me->count_n[count_idx] != 0) {}
            // kind of like moving a global pointer to where to insert across processes
        }
        
        // actually do work!
        while(me->count_n[count_idx] > 0) {
            printf("%i moving %8x into %i->[%i]\n", ta->my_tid, lower->records[lower_idx].key, *ta->add_thread,  *ta->add_idx);
            thread_mem[*ta->add_thread].arr_start[*ta->add_idx] = lower->records[lower_idx];
            lower_idx += 1;
            me->count_n[count_idx] -= 1;
            if(*ta->add_idx == ta->ARR_SIZE - 1) {
                *ta->add_idx = 0;
                // mark that thread as filled so it can start sort!
                thread_mem[*ta->add_thread].filled = 1;
                *ta->add_thread += 1;
            } else {
                *ta->add_idx += 1;
            }
            me->inserted += 1;
        }
        // printf("TID:%2i\n\t FINISHED: %2i\n", ta->my_tid, count_idx);
        count_idx += 1;
    }
    return 0;
}

void* t_run(void* in_ta) {
    // bullshit C setup
    thread_args* ta = (thread_args*) in_ta;
    typedef struct t_lower
    {
        record records[ta->ARR_SIZE];
        record* old_pointers[ta->ARR_SIZE];
    } t_lower;
    t_lower* lower = malloc(sizeof(t_lower));
    if(!lower) {
        printf("failed to allocate lower arr");
        exit(1);
    }

    t_radix* thread_mem = (t_radix *) ta->thread_mem;
    t_radix* me = &thread_mem[ta->my_tid];
    
    // sort 
    //for(int r = 0; r < KEY_BITS / BITS_AT_ONCE; r++) {
    for(int r = 0; r < 1; r++) {
        // printf("iteration: %i\n", r);
        // for(int i = 0; i < ta->ARR_SIZE; i++) {
        //     printf("m%i\t%p\t%8x\n", ta->my_tid, &me->arr_start[i], me->arr_start[i].key);
        // }
        // printf("%2i sorting...\n", ta->my_tid);
        me->sorted = 0;
        counting_sort(me->arr_start, ta->ARR_SIZE, lower, me->count_n, r);
        me->sorted = 1;
        // printf("\n");
        // for(int i = 0; i < ta->ARR_SIZE; i++) {
        //     printf("%i\t%p\t%8x\n", ta->my_tid, lower->old_pointers[i], lower->records[i].key);
        // }
        move_to_mem(thread_mem, lower, ta);
        // msync(thread_mem[0].arr_start, ta->filesize, MS_SYNC);
        
    }
    
    // for(int i = 0; i < ta->ARR_SIZE; i++) {
    //     printf("%i\t%p\t%8x\n", ta->my_tid, lower->old_pointers[i], lower->records[i].key);
    // }
    // printf("\n");
    
    free(lower);
    return 0;
}