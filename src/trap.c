/**
 * Kevin Lin and Carter Sullivan
 * Yalnix
 * CS58 - Operating Systems 24W
 *
 * trap.c
 *
 */

#include <hardware.h>
#include <yalnix.h>
#include "kernel.h"
/* Creates a new process that is a copy of the calling process. */
int Y_Fork(void);
/* Replaces the currently running process with a new process image. */
int Y_Exec(char *filename, char *argv[]);
/* Terminates the calling process with exit code {status} */
int Y_Exit(int status);
/* Waits for a child process to terminate, returns the child's PID and stores status at ptr. */
int Y_Wait(int *status);
/* Returns the PID of the calling process. */
int Y_Getpid(void);
/* Changes the location of the program break. */
int Y_Brk(void *addr);
/* Delays the calling process for a specified number of clock ticks. */
int Y_Delay(int clock_ticks);
int Y_Ttyread(int tty_id, void *buf, int len);
int Y_Ttywrite(int tty_id, void *buf, int len);
int Y_Pipeinit(int *pipe_idp);
int Y_Piperead(int pipe_id, void *buf, int len);
int Y_Pipewrite(int pipe_id, void *buf, int len);
int Y_LockInit(int *lock_idp);
int Y_Acquire(int lock_id);
int Y_Release(int lock_id);
int Y_CvarInit(int *cvar_idp);
int Y_CvarSignal(int cvar_id);
int Y_CvarBroadcast(int cvar_id);
int Y_Cvarwait(int cvar_id, int lock_id);
int Y_Reclaim(int id);

void TrapKernel(UserContext *user_context)
{
    TracePrintf(1, "TRAPPPPP: Kernel Trap. Syscall code: %d\n", user_context->code);
    memcpy(&current_process->user_c, user_context, sizeof(UserContext));

    /**
     * 1. Save user context
     * 2. Switch to kernel mode
     * 3. Use the syscall code to call the relevant syscall
     * 4. Restore user context
     * 5. Return to user mode, return value in r0
     */

    int code = user_context->code;
    int r_value;

    switch (code)
    {
    case YALNIX_BRK:
        TracePrintf(1, "TRAP BRK REG; 0: %p\n", (void *)user_context->regs[0]);
        r_value = Y_Brk((void*) user_context->regs[0]);
        break;
    case YALNIX_FORK:
        r_value = Y_Fork();
        break;
    case YALNIX_EXEC:
        TracePrintf(1, "TRAP EXEC REG; 0: %s, 1: %s\n", (char *)user_context->regs[0], (char **)user_context->regs[1]);
        r_value = Y_Exec((char *)user_context->regs[0], (char **)user_context->regs[1]);
        break;
    case YALNIX_DELAY:
        TracePrintf(1, "TRAP DELAY REG; 0: %d\n", (int)user_context->regs[0]);
        r_value = Y_Delay((int)user_context->regs[0]);
        break;
    case YALNIX_GETPID:
        r_value = Y_Getpid();
        break;
    case YALNIX_WAIT:
        r_value = Y_Wait((void*)user_context->regs[0]);
        TracePrintf(1, "return value for yalnix wait: %d\n", r_value);
        break;
    case YALNIX_EXIT:
        TracePrintf(1, "TRAP EXIT REG; 0: %d\n", (int)user_context->regs[0]);
        Y_Exit((int)user_context->regs[0]);
        break;
    default:
        break;
    }
    memcpy(user_context, &current_process->user_c, sizeof(UserContext));
    user_context->regs[0] = r_value;
    TracePrintf(1, "Exiting kernel trap\n");
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

    int terminal_num = user_context->code;
    void* address_to_copy_to = malloc(TERMINAL_MAX_LINE);

    int input_length = TtyReceive(terminal_num, address_to_copy_to, TERMINAL_MAX_LINE);
    Node_t* new_node = createNode(address_to_copy_to);
    enqueue(terminal_input_buffers[terminal_num], new_node);
    
    while(input_length == TERMINAL_MAX_LINE){
        address_to_copy_to = malloc(TERMINAL_MAX_LINE);
        input_length = TtyReceive(terminal_num, address_to_copy_to, TERMINAL_MAX_LINE);
        Node_t* new_node = createNode(address_to_copy_to);
        enqueue(terminal_input_buffers[terminal_num], new_node);
    }
}

void TrapTTYTransmit(UserContext *user_context)
{
    /**
     * Set can_transmit_to_terminal to true for the relevant terminal
     */

    int terminal_num = user_context->code;

    can_write_to_terminal[terminal_num] = 1;


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
