/**
 * Kevin Lin and Carter Sullivan
 * Yalnix
 * CS58 - Operating Systems 24W
 *
 * trap.c
 * 
 */

#include <hardware.h>

void TrapKernel(UserContext *user_context)
{
    TracePrintf(1, "TRAPPPPP: Kernel Trap. Syscall code: %d\n", user_context->code);
    /**
     * 1. Save user context
     * 2. Switch to kernel mode
     * 3. Use the syscall code to call the relevant syscall
     * 4. Restore user context
     * 5. Return to user mode, return value in r0
    */
}

void TrapClock(UserContext *user_context)
{
    TracePrintf(1, "TRAPPPPP: Clock Trap.\n");
    /**
     * 1. Increment clock (if we go with a global clock)
     * 2. Check delay queue, find all processes that are ready to be woken up
     *       a. This delay queue is sorted by delay()
     * 3. Move these processes from delay queue to ready queue
     * 4. return to user mode
    */
}

void TrapIllegal(UserContext *user_context)
{
    /**
     * 1. Kill cur process with Y_Exit(ILLEGAL_INSTRUCTION)
     * 2. Return to user mode
    */
}

void TrapMemory(UserContext *user_context)
{
    /**
     * 1. If invalid address (not in region 0 or 1), exit ILLEGAL_MEMORY
     * 2. If invalid permissions code, exit ILLEGAL_MEMORY
     * 3. If address not mapped code, we need to map it because the user thinks it exists
     *    a. Grow stack to this address it doesnt pass heap
     *    b. But I think if it passes the heap, it probably was a valid address?
    */
}

void TrapMath(UserContext *user_context)
{
    /**
     * 1. Kill cur process with Y_Exit(FLOATING_POINT_EXCEPTION)
     * 
    */   
}


void TrapTTYReceive(UserContext *user_context)
{
    /**
     * while there are more bytes to be read in:
     *      Allocate buf of size MAXIMUM_TERMINAL_INPUT on kernel heap
     *      Make linked list node storing the buf, add it to relevant user's terminal input buffer
     *      Execute ttyReceive machine instruction to read user input into buf, with max len of MAXIMUM_TERMINAL_INPUT. 
     *      
    */
}

void TrapTTYTransmit(UserContext *user_context)
{
    /**
     * Set can_transmit_to_terminal to true for the relevant terminal
    */
}

void TrapDisk(UserContext *user_context)
{
    /**
     * 1. NOT USED FOR NOW
    */
}

void TrapElse(UserContext *user_context)
{
    TracePrintf(1, "TRAPPPPP: General Exception Trap.\n");
    /**
     * 1. Kill cur process with Y_Exit(GENERAL_EXCEPTION)
    */
}
