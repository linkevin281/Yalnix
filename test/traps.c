#include <hardware.h>
#include "../src/kernel.h"
#include <yuser.h>

// Testing the mem trap. Lets try to read from a location that is not mapped
int mem_trap_grow()
{
    TracePrintf(1, "TEST START: mem_trap_grow. Trying to grow the stack by reading from an unmapped location\n");

    int stackfinder = 0xdeadbeef;
    TracePrintf(1, "Current stack pointer: %p, current brk: %p\n", &stackfinder, sbrk(0));

    TracePrintf(1, "Trying to read from an unmapped location in the middle of userland\n");
    int *ptr = (int *)0x1d0000;
    int val = *ptr;
    TracePrintf(1, "Did we survive? We should have trapped\n");

    TracePrintf(1, "TEST END: mem_trap_grow\n");
    return 0;
}

// This just segfaults.
int mem_trap_exec()
{
    TracePrintf(1, "TEST START: mem_trap_exec. Trying to execute a fake function from an unmapped location\n");
    int stackfinder = 0xdeadbeef;
    TracePrintf(1, "Current stack pointer: %p, current brk: %p\n", &stackfinder, sbrk(0));

    void (*func)(void) = (void *)0x1c500;
    TracePrintf(1, "Trying to execute a fake function from an unmapped location\n");
    func();
    TracePrintf(1, "Did we survive? We should probably have trapped with some YALNIX_ACCERR error\n");

    TracePrintf(1, "TEST END: mem_trap_exec\n");
}

// We want the brk at 0x146000 but it grows to 0x14e000. ???
int mem_trap_heap()
{
    TracePrintf(1, "TEST START: mem_trap_heap. Trying to read from a location that is too close to the heap\n");
    int stackfinder = 0xdeadbeef;
    TracePrintf(1, "Current stack pointer: %p, current brk: %p\n", &stackfinder, sbrk(0));

    TracePrintf(1, "Growing the heap by a few pages. Assuming our brk was at 0x10a000. Lets grow it by 30 pages\n");
    int *big_arr = malloc(30 * PAGESIZE);
    TracePrintf(1, "We want our brk to be at 0x10a000 + 30*pagesize = %p\n", (void *)(0x10a000 + 30 * PAGESIZE));
    TracePrintf(1, "New brk from user prog is: %p\n", sbrk(0));

    TracePrintf(1, "Trying to grow stack to the new brk. AKA: trying to read from a location that is too close to the heap.\n");
    TracePrintf(1, "Expecting to fail at REDZONE_SIZE above the brk page (which is almost like 1+REDZONE_SIZE). Failure dist: %d\n", REDZONE_SIZE + 1);
    for (int dist = 5; dist > -1; dist--)
    {
        TracePrintf(1, "Dist = %d. Trying to read from %p\n", dist, sbrk(0) + dist * PAGESIZE);
        int *ptr = (int *)(sbrk(0) + dist * PAGESIZE);
        int val = *ptr;
        TracePrintf(1, "Did we survive? We should have trapped\n");
    }
}

int mem_trap_bounds_up()
{
    TracePrintf(1, "TEST START: mem_trap_bounds. Trying to read from a location that is not in userland\n");
    int stackfinder = 0xdeadbeef;
    TracePrintf(1, "Current stack pointer: %p, current brk: %p\n", &stackfinder, sbrk(0));

    TracePrintf(1, "Trying to read from a location that is not in userland\n");
    int *ptr = (int *)0x200001;
    int val = *ptr;
    TracePrintf(1, "Did we survive? We should have trapped\n");
}

int mem_trap_bounds_down()
{
    TracePrintf(1, "TEST START: mem_trap_bounds. Trying to read from a location that is not in userland\n");
    int stackfinder = 0xdeadbeef;
    TracePrintf(1, "Current stack pointer: %p, current brk: %p\n", &stackfinder, sbrk(0));

    TracePrintf(1, "Trying to read from a location that is not in userland\n");
    int *ptr = (int *)0x0a0000;
    int val = *ptr;
    TracePrintf(1, "Did we survive? We should have trapped\n");
}

void trap_math() 
{
    TracePrintf(1, "TEST START: trap_math. Trying to divide by zero\n");
    int a = 1;
    int b = 2;
    int c = a / b;
    TracePrintf(1, "c: %d\n", c);
    int d = c / 0;
    TracePrintf(1, "UHOH SPAGETTIOS: WE SHOULD HAVE DIED\n");
}

int main()
{
    int child1 = Fork();
    if (child1 == 0)
    {
        TracePrintf(1, "TEST: trap_math. I am the child\n");
        trap_math();
        return 0;
    }
    else
    {
        TracePrintf(1, "TEST: trap_math. I am the parent\n");
    }

    mem_trap_grow();
    // mem_trap_exec();
    // mem_trap_bounds_up();
    mem_trap_bounds_down();
    // mem_trap_heap();
    return 0;
}