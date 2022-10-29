// Record Layout
// |----key----|-------------------------value-------------------------|
// |  4 bytes  |                        96 bytes                       |

// Want to parellel sort on key
// key looks like:
//    3B        2F        08        2A
// 00111011  00101111  00001000  00101010


// Unknown length of input (maybe can get line info from file header or something??)
//      Create n processes
//      Modulo add records into n contiguous buckets
//      radix sort on buckets with n processors
//      (???) share memory

// Radix sort


int sort(int** arr); 