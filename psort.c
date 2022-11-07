#include "sort.h"

int main(int argc, char* argv[]) {
    if(argc < 3) {
        return -1;
    }
    p_radix_sort(argv[1], argv[2]);

    return 0;
}