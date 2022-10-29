#include "sort.h"
#include "stdlib.h"

int test_sort(int n) {
    int** arr = malloc(n*sizeof(int*));
    int j = 0;
    for(int i = n; i >= 0; i--) {
        arr[j] = malloc(sizeof(int*));
        *arr[j] = i;
        j++;
    }
    sort(arr);
    for(int i = 0; i < n; i++) {
        free(arr[i]);
    }
    free(arr);
    return 0;
}

int main(int argc, char* argv[]) {
    int n = atoi(argv[1]);
    test_sort(n);
    return 0;
}