#include "sort.h"
#include "stdio.h"
#include "stdlib.h"
#include "thread.h"
#include "string.h"
#include "sys/mman.h"
#include "unistd.h"
#include "pthread.h"
#include "stdatomic.h"

uint32_t flip_sign(key_t key) {
    return key ^ 0x80000000;
}

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
int counting_sort(record* start, int size, record* lower, int iteration) {
    key_t mask = (NUM_POS_VALUES-1);
    int C[NUM_POS_VALUES];
    mask = mask << (iteration*BITS_AT_ONCE);
    for(int j = 0; j<NUM_POS_VALUES; j++) {
        C[j] = 0;
    }
    // count occurences of mask
    for(int i = 0; i<size; i++) {
        // eg. C[0xF & key] += 1
        C[(unsigned)(flip_sign(start[i].key) & mask) >> (iteration*BITS_AT_ONCE)] += 1;
        // IMPORTANT TO USE UNSIGNED
    }

    // get sum-up_to[j]
    // C[0] = count_n[0];
    for(int j = 1; j<NUM_POS_VALUES; j++) {
        C[j] = C[j] + C[j-1];
    }
    // final sort
    for(int k = size-1; k>=0; k--) {
        C[(unsigned)(flip_sign(start[k].key) & mask) >> (iteration*BITS_AT_ONCE)] -= 1;
        lower[C[(unsigned)(flip_sign(start[k].key) & mask) >> (iteration*BITS_AT_ONCE)]] = start[k];
    }

    // for(int i = 0; i < NUM_POS_VALUES; i++) {
    //     atomic_fetch_add_explicit(&s_count->remaining[i], count_n[i], memory_order_acq_rel);
    // }
    return 0;
}


// int move_to_mem(t_radix* thread_mem, record* lower, thread_args* ta) {
//     t_radix* me = &thread_mem[ta->my_tid];
//     shared_memory* s_mem = ta->s_memory;
//     shared_count* s_count = ta->s_count;
//     globals* global = ta->global;
//     int tid = ta->my_tid;
//     int count_idx = 0;
//     int lower_idx = 0;
    

    
//     // reset idxes each round
//     if(tid == 0) {
//         while(1) {
//             if(pthread_mutex_lock(s_mem->lock) == -1) {
//                 pthread_cond_wait(s_mem->checkable, s_mem->lock);
//             }
//             if(s_mem->t_turn % THREADS == tid) {
//                 s_mem->c_t_arr = 0;
//                 s_mem->c_t_idx = 0;
//                 break;
//             }
//             pthread_mutex_unlock(s_mem->lock);
//         }
//         pthread_mutex_unlock(s_mem->lock);
//     }

//     while(count_idx < NUM_POS_VALUES) {
//         // wait for turn
//         while(1) {
//             // try to lock, if fail, wait
//             if(pthread_mutex_lock(s_mem->lock) == -1) {
//                 pthread_cond_wait(s_mem->checkable, s_mem->lock);
//             }
//             if(s_mem->t_turn % THREADS == tid) {
//                 break;
//             }
//             pthread_mutex_unlock(s_mem->lock);
//         }

//         // insert here!
//         int inserting = me->count_n[count_idx];
//         while(me->count_n[count_idx] != 0) {
//             // check for zeros (might not work in tests ) TODO
//             if(tid == THREADS - 1 && lower[lower_idx].key == 0) {
//                 ;;
//             } else {
//                 // add into curr ptr!
//                 thread_mem[s_mem->c_t_arr].arr_put[s_mem->c_t_idx] = lower[lower_idx];
//             }
//             for(int i = count_idx; i < NUM_POS_VALUES; i++) {
//                 me->count_n[i] -= 1;
//             }
            
//             lower_idx += 1;
//             // calc new idx
//             if(s_mem->c_t_idx == global->ARR_SIZE - 1) {
//                 s_mem->c_t_idx = 0;
//                 s_mem->c_t_arr += 1;
//             } else {
//                 s_mem->c_t_idx += 1;
//             }
//         }

//         for(int i = count_idx; i < NUM_POS_VALUES; i++) {
//             atomic_fetch_sub(&s_count->remaining[i], inserting);
//         }
//         count_idx += 1;
//         // printf("[%i] unlocked the lock!\n", tid);

//         s_mem->t_turn += 1;
//         pthread_cond_broadcast(s_mem->checkable);
//         pthread_mutex_unlock(s_mem->lock);
//         me->filled = count_idx;
//     }
//     return 0;
// }

// returns count added, if fail -1
int get_msb_like_me(record* lower, record* readin, record** extras, unsigned int tid, int total_records, int num_in_lower, int* extras_made) {
    int total_count = 0;
    int lower_count = 0;
    record* curr_extra = NULL;
    int curr_extra_count = 0;
    int curr_extra_idx = 0;

    
    for(int i = 0; i < total_records; i++) {
        // printf("%i, %X, %p\n", i, KEY_MASK, readin);

        if(((KEY_MASK & flip_sign(readin[i].key)) >> 28) == tid) {
            // printf("%08x, %08X, %08X, %X\n", readin[i].key, readin[i].key & KEY_MASK, (readin[i].key & KEY_MASK) >> 28, tid);
            if(lower_count >= num_in_lower) {
                // set up new array in extras if needed
                if(curr_extra_count >= EXTRA_SIZE || !curr_extra) {
                    // printf("%i made extra \n", tid);
                    curr_extra = (record*) malloc(EXTRA_SIZE * sizeof(record));
                    if(!curr_extra) {
                        printf("failed to alloc extra");
                    }
                    memset(curr_extra, 0, EXTRA_SIZE * sizeof(record));
                    extras[curr_extra_idx] = curr_extra;
                    curr_extra_idx += 1;
                    curr_extra_count = 0;
                    
                }
                // printf("putting %X in %p[%i] for %X\n", readin[i].key, curr_extra, curr_extra_count, tid);
                // populate curr_extra array
                curr_extra[curr_extra_count] =  readin[i];
                curr_extra_count += 1;
                total_count += 1;
            } else {
                
                lower[lower_count] = readin[i];
                lower_count += 1;
                total_count += 1;
            }
        }
    }
    return total_count;
}
// get idx in out to where thread should write
int get_out_index(int total_in, shared_memory* s_mem, int tid) {
    int out = -1;
    while(1) {
        // try to lock, if fail, wait
        if(pthread_mutex_lock(s_mem->lock) == -1) {
            pthread_cond_wait(s_mem->checkable, s_mem->lock);
        }
        if(s_mem->t_turn % THREADS == tid) {
            break;
        }
        pthread_mutex_unlock(s_mem->lock);
    }
    // LOCKED
    out = s_mem->curr_idx;
    s_mem->curr_idx += total_in;
    s_mem->t_turn += 1;
    pthread_cond_broadcast(s_mem->checkable);
    pthread_mutex_unlock(s_mem->lock);
    // UNLOCKED

    return out;
}

void* t_run(void* in_ta) {
    // bullshit C setup
    thread_args* ta = (thread_args*) in_ta;

    // extract ta
    globals* global = ta->global;
    // shared_count* s_count = ta->s_count;
    shared_memory* s_memory = ta->s_memory;
    t_radix* thread_mem = ta->thread_mem;
    // CHANGE
    // ta->my_tid = 1;

    t_radix* me = &thread_mem[ta->my_tid];

    // setup lower
    int num_in_lower = global->total_records / THREADS;
    if(num_in_lower < MIN_LOWER) {num_in_lower = MIN_LOWER;}

    record* lower = (record*) malloc(num_in_lower * sizeof(record));
    if(!lower) {
        printf("failed to alloc lower");
        exit(EXIT_FAILURE);
    }
    memset(lower, 0, num_in_lower * sizeof(record));

    record** extras = (record**) malloc(NUM_EXTRAS_POS * sizeof(record*));
     if(!extras) {
        printf("failed to alloc extras");
        exit(EXIT_FAILURE);
    }

    // get my childern and count
    // put in lower, then read/write out buffer
    int extras_made = -1;
    int total_in_me = get_msb_like_me(lower, me->arr_start, extras, me->my_tid, global->total_records, num_in_lower, &extras_made);

    // for(int i = 0; i < num_in_lower; i++) {
    //     printf("%X:\t%08X\n", me->my_tid, lower[i].key);
    // }

    // get idx in out arr
    int out_start = get_out_index(total_in_me, s_memory, me->my_tid);

    // get pointer to my section
    record* out_arr = &me->out[out_start];

    // put all vals into mem
    int curr_extra_idx = 0;
    int added_from_extra = 0;
    for(int i = 0; i < total_in_me; i++) {
        if(i >= num_in_lower) {
            if(added_from_extra >= EXTRA_SIZE) {
                curr_extra_idx += 1;
                added_from_extra = 0;
            }
            out_arr[i] = extras[curr_extra_idx][added_from_extra];
            added_from_extra += 1;
        } else {
            out_arr[i] = lower[i];
        }
    }


    // printf("HEY!\n");


    // free lower and extras
    if(extras_made > 0) {
        for(int i = 0; i < extras_made; i++) {
            free(extras[i]);
        }
    }
    free(extras);
    free(lower);



    // create new lower of proper size
    record* fit_lower = (record*) malloc(total_in_me * sizeof(record));
    if(!lower) {
        printf("failed to alloc lower");
        exit(EXIT_FAILURE);
    }
    memset(fit_lower, 0, total_in_me * sizeof(record));

    // actually sort
    for(int r = 0; r < KEY_BITS / BITS_AT_ONCE; r++) {
        if(r % 2 == 0) {
            counting_sort(out_arr, total_in_me, fit_lower, r);
        } else {
            counting_sort(fit_lower, total_in_me, out_arr, r);
        }
    }

    free(fit_lower);
    return 0;

}