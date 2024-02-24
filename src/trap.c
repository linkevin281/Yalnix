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
        r_value = Y_Brk((void *)user_context->regs[0]);
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
        r_value = Y_Wait((void *)user_context->regs[0]);
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

/**
 * When a clock interrupt occurs, the hardware generates a TRAP CLOCK interrupt. The periodic
 *   frequency of clock interrupts is adjustable at the command line when you invoke yalnix (see Section 5.4.3).
 *
 * Process:
 *   1. Increment clock_ticks
 *   2. Save user context
 *   3. Run the next process
 *   4. Restore user context
 *
 */
void TrapClock(UserContext *user_context)
{
    TracePrintf(1, "TRAPPPPP: Clock Trap.\n");
    clock_ticks++;
    memcpy(&current_process->user_c, user_context, sizeof(UserContext));
    runProcess();
    memcpy(user_context, &current_process->user_c, sizeof(UserContext));
    current_process->state = RUNNING;
}

/**
 * This exception results from the execution of an illegal instruction by the currently executing
 *   user process. An illegal instruction can be an undefined machine language opcode, an illegal addressing mode,
 *   or a privileged instruction when not in kernel mode.
 *
 * Process:
 *   1. Kill cur process with Y_Exit(ILLEGAL_INSTRUCTION)
 *   2. Return to user mode
 */
void TrapIllegal(UserContext *user_context)
{
    TracePrintf(1, "TRAPPPPP: Illegal Instruction Trap.\n");
    Y_Exit(ILLEGAL_INSTRUCTION);
}

/**
 * This exception results from a disallowed memory access by the current user process. The
 *   access may be disallowed because the address is outside the virtual address range of the hardware (outside
 *   Region 0 and Region 1), because the address is not mapped in the current page tables, or because the access
 *   violates the page protection specified in the corresponding page table entry.
 *   Your handler should figure out what to do based on the faulting address.
 *
 * Process:
 *   0. Inspect code to see what the problem is.
 *   1. If addr is not in userland, exit ILLEGAL_MEMORY
 *   2. If addr is too close to the heap, exit ILLEGAL_MEMORY
 *   3. If addr is not mapped, map up to addr
 */
void TrapMemory(UserContext *user_context)
{
    TracePrintf(1, "TRAPPPPP: Memory Trap. Address: %p\n", (void *)user_context->addr);
    TracePrintf(1, "FYI: Actual Brk is at: %p\n", (void *)current_process->brk + VMEM_0_SIZE);

    int code = user_context->code;
    int addr = (unsigned int)user_context->addr;

    // 0. Inspect code to see what the problem
    if (code == YALNIX_ACCERR)
    {
        TracePrintf(1, "ERROR: Access not allowed. Got code %d: Invalid permissions. Addr: %p\n", code, (void *)user_context->addr);
        Y_Exit(ILLEGAL_MEM_ADDR);
    }
    else if (code == YALNIX_MAPERR)
    {
        TracePrintf(1, "UHOH: Address not mapped. Got code %d: Considering growing the stack... Addr: %p\n", code, (void *)user_context->addr);
    }
    else
    {
        return;
    }

    // 1. If addr is not in userland, exit ILLEGAL_MEMORY
    if (addr < VMEM_1_BASE || addr >= VMEM_1_LIMIT)
    {
        TracePrintf(1, "ERROR: Address not in userland. Bottom: %p, Top: %p, Addr: %p\n", (void *)VMEM_1_BASE, (void *)VMEM_1_LIMIT, (void *)addr);
        Y_Exit(ILLEGAL_MEM_ADDR);
    }
    else
    {
        int page = (DOWN_TO_PAGE(addr) - VMEM_0_SIZE) >> PAGESHIFT;
        int heap_limit_page = (current_process->brk >> PAGESHIFT) + REDZONE_SIZE + 1;
        // 2. Are we trying to read too close to heap? THIS INCLUDES THE REDZONE PAGE.
        if (page <= heap_limit_page)
        {
            TracePrintf(1, "ERROR: Address is too close to heap. Addr_p: %d, brk+p: %d, Heap+Redzone_p: %d, Redzone: %d\n", page, current_process->brk, heap_limit_page, REDZONE_SIZE);
            Y_Exit(ILLEGAL_MEM_ADDR);
        }

        // 3. Ok. If we get here, we can access this page. But is it mapped?
        pte_t *pt = current_process->userland_pt;
        if (pt[page].valid == 0)
        {
            TracePrintf(1, "UHOH: User thinks this addr exists but we haven't mapped it yet. Page: %d. Mapping them now.\n", page);
            // Loop from the top of mem to the page we need to map
            for (int pg = MAX_PT_LEN - 1; pg >= page; pg--)
            {
                // Skip if already valid
                if (pt[pg].valid == 1)
                {
                    TracePrintf(1, "P: %d already valid\n", pg);
                    continue;
                }
                int frame = allocateFrame();
                if (frame == -1)
                {
                    TracePrintf(1, "ERROR: Out of memory. Exiting.\n");
                    Y_Exit(EXECPTION_OUT_OF_MEM);
                }
                pt[pg].valid = 1;
                pt[pg].prot = PROT_READ | PROT_WRITE;
                pt[pg].pfn = frame;
                TracePrintf(1, "P: %d -> F: %d\n", pg, frame);
            }
            TracePrintf(1, "Mapped up to page %d, addr: %p\n", page, (void *)(page << PAGESHIFT));
        }
    }
}

/**
 * This exception results from any arithmetic error from an instruction executed by the current user
 *   process, such as division by zero or an arithmetic overflow
 *
 * Process:
 *   1. Kill cur process with Y_Exit(FLOATING_POINT_EXCEPTION)
 */
void TrapMath(UserContext *user_context)
{
    TracePrintf(1, "TRAPPPPP: Math Trap.\n");
    Y_Exit(FLOATING_POINT_EXCEPTION);
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

/**
 * This interrupt is generated by the disk when it completes an operation (as mentioned in Section 2.3.3 above).
 */
void TrapDisk(UserContext *user_context)
{
    /**
     * 1. NOT USED FOR NOW
     */
}

/**
 * This trap handler handles all else.
 *
 * Process:
 *   1. Kill cur process with Y_Exit(GENERAL_EXCEPTION)
 */
void TrapElse(UserContext *user_context)
{
    TracePrintf(1, "TRAPPPPP: General Exception Trap.\n");
    Y_Exit(GENERAL_EXCEPTION);
}
