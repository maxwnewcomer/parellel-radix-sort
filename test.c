#include "sort.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "time.h"


int getrand(int min,int max){
    return(rand()%(max-min)+min);
}

char *gen_rdm_bytestream (size_t num_bytes)
{
    char *stream = malloc (num_bytes);
    size_t i;
    for (i = 0; i < num_bytes; i++)
    {
        stream[i] = getrand(0, 255);
    }
    return stream;
}


int generate_test_file(int n, char* filename) 
{
    FILE * f;
    f = fopen(filename, "w");
    if(f == NULL)
        exit(EXIT_FAILURE);
    for(int i = 0; i < n; i++) {
        char* record = (char*)gen_rdm_bytestream(100);
        while(strlen(record)!=100) {
            record = (char*)gen_rdm_bytestream(100);
        }
        fputs(record, f);
        free(record);
    }
    fclose(f);
    return 0;
}

int main(int argc, char* argv[]) 
{
    srand(time(NULL));

    int n;
    if(argc < 2) {
        n = 10;
    } else {
        n = atoi(argv[1]);
    }
    printf("Writing %i records...\n", n);
    char filename[] = "records.dat";

    generate_test_file(n, filename);
    // start timer
    clock_t start = clock(), diff;
    // sort
    

    p_radix_sort(filename);
    // get perf into
    diff = clock() - start;
    int msec = diff * 1000 / CLOCKS_PER_SEC;
    printf("Time taken %d seconds %d milliseconds\n", msec/1000, msec%1000);
    
    return 0;
}