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
    // count occurences of mask
    for(int i = 0; i<size; i++) {
        // eg. C[0xF & key] += 1
        count_n[(unsigned)(start[i].key & mask) >> (iteration*BITS_AT_ONCE)] += 1;
        // IMPORTANT TO USE UNSIGNED
    }

    // get sum-up_to[j]
    C[0] = count_n[0];
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
    t_radix* me = &thread_mem[ta->my_tid];
    shared_memory* s_mem = ta->s_memory;
    shared_count* s_count = ta->s_count;
    globals* global = ta->global;
    int tid = ta->my_tid;
    int count_idx = 0;
    int lower_idx = 0;
    // reset idxes each round
    if(tid == 0) {
        while(1) {
            pthread_mutex_lock(s_mem->lock);
            if(s_mem->t_turn % global->THREADS == 0) {
                s_mem->c_t_arr = 0;
                s_mem->c_t_idx = 0;
                break;
            }
            pthread_mutex_unlock(s_mem->lock);
        }
        pthread_mutex_unlock(s_mem->lock);
    }
   
    


    // DEBUG
    // printf("\n[%i]\n", tid);
    // for(int i = 0; i < NUM_POS_VALUES; i++) {
    //     printf("%2i ", me->count_n[i]);
    // }
    // printf("\n");
    //

    while(count_idx < NUM_POS_VALUES) {
        // if thread 0 on first iter, try to get lock and run
        // if(tid == 0 && count_idx == 0) {
        //     while(1) {
        //         pthread_mutex_lock(s_mem->lock);
        //         if(s_mem->t_turn % global->THREADS == tid) {
        //             break;
        //         }
        //         pthread_mutex_unlock(s_mem->lock);
        //     }
        // } else {
            // wait for turn
        while(1) {
            // try to lock, if fail, wait
            if(pthread_mutex_lock(s_mem->lock) == -1) {
                pthread_cond_wait(s_mem->checkable, s_mem->lock);
            }
            if(s_mem->t_turn % global->THREADS == tid) {
                break;
            }
            pthread_mutex_unlock(s_mem->lock);
        }
        // }
        
        // get lock
        // pthread_mutex_lock(s_mem->lock);
        // printf("[%i] has the lock!\n", tid);
        // reset global ptr at start of each round

        // insert here!
        int inserting = me->count_n[count_idx];
        while(me->count_n[count_idx] != 0) {
            // check for zeros (might not work in tests ) TODO
            if(tid == global->THREADS - 1 && lower[lower_idx].key == 0) {
                ;;
            } else {
                // add into curr ptr!
                thread_mem[s_mem->c_t_arr].arr_start[s_mem->c_t_idx] = lower[lower_idx];
            }
            for(int i = count_idx; i < NUM_POS_VALUES; i++) {
                me->count_n[i] -= 1;
            }
            
            lower_idx += 1;
            // calc new idx
            if(s_mem->c_t_idx == global->ARR_SIZE - 1) {
                s_mem->c_t_idx = 0;
                s_mem->c_t_arr += 1;
            } else {
                s_mem->c_t_idx += 1;
            }
        }

        for(int i = count_idx; i < NUM_POS_VALUES; i++) {
            atomic_fetch_sub(&s_count->remaining[i], inserting);
        }
        count_idx += 1;
        // printf("[%i] unlocked the lock!\n", tid);

        s_mem->t_turn += 1;
        pthread_cond_broadcast(s_mem->checkable);
        pthread_mutex_unlock(s_mem->lock);
        me->filled = count_idx;
    }
    return 0;
}

void* t_run(void* in_ta) {
    // bullshit C setup
    thread_args* ta = (thread_args*) in_ta;

    // extract ta
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
    for(int r = 0; r < KEY_BITS / BITS_AT_ONCE; r++) {
        counting_sort(me->arr_start, global->ARR_SIZE, lower, me->count_n, s_count, r);
        move_to_mem(thread_mem, lower, ta);
    }
    free(lower);
    return 0;

}