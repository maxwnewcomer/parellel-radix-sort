#include "sort.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
unsigned char *gen_rdm_bytestream (size_t num_bytes)
{
    unsigned char *stream = malloc (num_bytes);
    size_t i;
    for (i = 0; i < num_bytes; i++)
    {
        stream[i] = rand();
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
        fputs(record, f);
        free(record);
    }
    fclose(f);
    return 0;
}

int main(int argc, char* argv[]) 
{
    int n;
    if(argc < 2) {
        n = 100;
    } else {
        n = atoi(argv[1]);
    }
    char filename[] = "records.dat";
    generate_test_file(n, filename);
    p_radix_sort(filename);
    return 0;
}