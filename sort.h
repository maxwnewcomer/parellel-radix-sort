// Record Layout
// |----key----|-------------------------value-------------------------|
// |  4 bytes  |                        96 bytes                       |

// Want to parellel sort on key
// key looks like:
//    3B        2F        08        2A
// 00111011  00101111  00001000  00101010


// Find number of record entries (n) (could reverse engineer testing file)
// Define number of processes (k) to create
// Create array of radix process structs
//      high: [n/k]
//      low: [n/k]
//      h_l: bool // indicates which array to insert into
//      filled: bool // indicates whether new array is filled
//      finished_0: bool // indicates whether process done filling in 
//                          its values into current insert array
//      my_idx: int // index in process array

// Program logic:
//      Get n
//      init p_radix arr
//      curr_bit = 0 (0th most sig bit)
//      readin keys into p_radix arr (high to start) (size 4B * n/k)
//      also read key->value pairs into dictionary
//      as p_radix[i]->high is filled spawn a thread
//      once thread done sorting p_radix[i]->high...
//          wait until p_radix[p_radix[i]->myidx - 1]->finished_0 (my_idx 0 fills into itself rightawy)
//          iterate p_radix[i] until p_radix[k]->filled = false
//          dump values into p_radix[k]->h_l (spilling over into next if necessary)
//          if spillover on fill or hit last index: swap h_l
//          wait until finished_0 and filled
//      once complete write key|dict[key] into new file going from p_radix[0]->p_radix[k]

// Globals
//      n - num records
//      arr_size - num keys per process
//      b - bit offset of key

#include "stdint.h"

#define T_ARR_SIZE 8
#define D_ARR_SIZE 1000
#define RECORD_SIZE 96
#define key_t int32_t


int sort(int** arr);

// radix sorter thread
struct t_radix
{
    key_t *high[T_ARR_SIZE];
    key_t *low[T_ARR_SIZE];
    int h_l;
    int filled;
    int finished_0;
    int my_idx;
};


// record in dictionary (linked list)
struct d_record 
{
    key_t key;
    char* record[RECORD_SIZE];
    struct d_record *next;
};