#include "sys/stat.h"
#include "stdint.h"
#include "fcntl.h"
#include "sys/mman.h"
#include "stdio.h"
#include "sort.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"


int get_file_size(char* filename) {
    struct stat st;
    stat(filename, &st);
    off_t size = st.st_size;
    return size;
}

int p_radix_sort(char* filename) {
    size_t pagesize = getpagesize();
    int filesize = get_file_size(filename);
    // get quick approximates
    int MAX_THREADS = filesize / 800; // can auto adjust this based on file size 
    if (MAX_THREADS > 10) MAX_THREADS = 10;
    int ARR_SIZE = filesize / 100 / MAX_THREADS * 1.35;
    // add struct definition (have to add here for dynamic arr size)
    typedef struct t_radix
    {
        int filled;
        int finished_0;
        int first_1;
        int my_tid;
        record* arr_start;
        record** lower[ARR_SIZE];
    } t_radix;

    t_radix *thread_arr = malloc(MAX_THREADS*sizeof(t_radix));
    memset(thread_arr, 0, MAX_THREADS*sizeof(t_radix));
    // memory map records
    struct stat sb;
    int f;
    f = open(filename, O_RDONLY);
    fstat(f, &sb);

    record* mapped_records = (record*) mmap ((void*) (pagesize * (1<<20)), filesize, PROT_READ || PROT_WRITE, MAP_SHARED, f, 0);
    if(mapped_records == MAP_FAILED) {
        printf("Failed to map memory :/\n");
        exit(EXIT_FAILURE);
    }

    // init structs
    for(int i = 0; i < MAX_THREADS; i++) {
        thread_arr[i].filled = 0;
        thread_arr[i].finished_0 = 0;
        thread_arr[i].my_tid = i;
        thread_arr[i].arr_start = &(mapped_records[i * ARR_SIZE]);
        memset(thread_arr[i].lower, 0, ARR_SIZE); // maybe unnecessary ?? 
    }
    printf("INIT Diagnostic:\n\tfilesize:   %i\n\tthreads:    %i\n\tARR_SIZE:   %i\n\tthreadsize: %li\n", filesize, MAX_THREADS, ARR_SIZE, sizeof(t_radix));
    // printf("%X\n", mapped_records[0].key);
    // printf("%p\n", thread_arr[0].arr_start);

    free(thread_arr);
    munmap(mapped_records, filesize);
    return 1;
}