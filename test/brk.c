/**
 * A file to test the "brk" syscall, which is called when a user calls malloc
*/

#include <hardware.h>
#include <yuser.h>
#include <stdio.h>

// Function to test basic allocation and memory access
void test_basic_allocation(size_t size) {
    TracePrintf(1,"Testing basic allocation with size %zu bytes...\n", size);

    char *data = (char *)malloc(size);
    if (data == NULL) {
        TracePrintf(1,"malloc failed to allocate %zu bytes\n", size);
        return;
    }

    // Test writing to the memory
    for (size_t i = 0; i < size; i++) {
        data[i] = 'a';
    }

    // Test reading from the memory
    for (size_t i = 0; i < size; i++) {
        if (data[i] != 'a') {
            TracePrintf(1,"Memory corruption detected at %zu\n", i);
            free(data);
            return;
        }
    }

    TracePrintf(1,"Basic allocation and access test passed for %zu bytes.\n", size);
    free(data);
}

// Function to test freeing memory and then reusing it
void test_free_and_reuse(size_t size) {
    TracePrintf(1,"Testing free and reuse with size %zu bytes...\n", size);

    char *data = (char *)malloc(size);
    if (!data) {
        TracePrintf(1,"malloc failed on initial allocation of %zu bytes\n", size);
        return;
    }

    free(data);

    // Attempt to reuse memory after free
    char *newData = (char *)malloc(size);
    if (!newData) {
        TracePrintf(1,"malloc failed on reuse after free for %zu bytes\n", size);
        return;
    }

    for (size_t i = 0; i < size; i++) {
        newData[i] = 'b';
    }

    TracePrintf(1,"Free and reuse test passed for %zu bytes.\n", size);
    free(newData);
}

// Function to test edge cases
void test_edge_cases() {
    TracePrintf(1,"Testing edge cases...\n");

    // Test allocation of 0 bytes
    void *zeroBytes = malloc(0);
    TracePrintf(1,"Successfully handled malloc with input zero, adress of pointer: %p.\n", zeroBytes);
    free(zeroBytes);

    // Test allocation of a very large amount of memory
    void *largeAlloc = malloc((size_t)-1); // This will attempt to allocate the maximum size_t value
    if (largeAlloc == NULL) {
        TracePrintf(1,"Large allocation test passed.\n");
    } else {
        TracePrintf(1,"malloc did not fail as expected with a very large size.\n");
        free(largeAlloc);
    }
}

int main() {
    // Run tests with various sizes
    test_basic_allocation(128); // Small allocation
    test_basic_allocation(1024); // 1MB allocation

    test_free_and_reuse(512); // Test freeing and reusing memory

    test_edge_cases(); // Test edge cases like 0 bytes and very large allocations

    return 0;
}