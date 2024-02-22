/**
 * A file to test the "brk" syscall, which is called when a user calls malloc
*/

#include <hardware.h>
#include <yuser.h>
#include <stdio.h>


void test_basic_allocation(int size) {
    TracePrintf(1, "In test_basic_allocation, attempting to allocate %d bytes\n", size);
    char *data = malloc(size);
    TracePrintf(1, "In test_basic_allocation, just allocated %d bytes at address %p\n", size, data);
    if (data == NULL) {
        TracePrintf(1,"failed to allocate %d bytes\n", size);
        return;
    }


    for (size_t i = 0; i < size; i++) {
        data[i] = 'a';
    }


    for (size_t i = 0; i < size; i++) {
        if (data[i] != 'a') {
            TracePrintf(1,"memory corrupted at index %d!\n", i);
            free(data);
            return;
        }
    }

    TracePrintf(1,"Allocation and access test passed with %d bytes - wooo!!!\n", size);
    free(data);
}


void test_edge_cases() {

    void *zeroes = malloc(0);
    TracePrintf(1,"Successfully handled malloc with input zero, adress of pointer: %p.\n", zeroes);
    free(zeroes);

    // Attempting to allocate a LOT of memory
    void *biggie = malloc((size_t)-1); 
    if (biggie == NULL) {
        TracePrintf(1,"Expected behavior with large malloc.\n");
    } else {
        TracePrintf(1,"Bad behavior with large attempted malloc\n");
        free(biggie);
    }
}

int main() {
    test_basic_allocation(128); 
    test_basic_allocation(20000);

    test_edge_cases(); 

    return 0;
}