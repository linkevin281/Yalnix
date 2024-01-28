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
    /**
     * 1. Save user context
     * 2. Switch to kernel mode
     * 3. Call syscall handler 
     * 4. Restore user context
     * 5. Return to user mode, return value in r0
    */
}

void TrapClock(UserContext *user_context)
{
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
     *    a. Grow stack to this address if it is in region 0
    */
}

void TrapMath(UserContext *user_context)
{
}

void TrapTTYReceive(UserContext *user_context)
{
}

void TrapTTYTransmit(UserContext *user_context)
{
}

void TrapDisk(UserContext *user_context)
{
}

void TrapElse(UserContext *user_context)
{
}
