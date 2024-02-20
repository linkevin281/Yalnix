/**
 * Kevin Lin and Carter Sullivan
 * Yalnix
 * CS58 - Operating Systems 24W
 *
 * syscalls.c
 *
 */

#include <hardware.h>
#include "syscalls.h"
#include "kernel.h"

// input: pcb
int Y_Fork()
{
    /**
     * Create new PCB for child process, with input PCB as its parent and user context copied in from input PCB
     * Update parent PCB to say that this new PCB is its child
     * Use KCCopy to copy parent's kernel context into the child
     * Initialize a new page table for the child process
     * For each page in the page table of the parent process:
     *     If page is valid:
     *         Grab a free frame from empty_frames
     *         allocate this free frame to the page
     *         Copy contents of the page in the old table to the child
     * Add child PCB to ready queue
     *
     */
}

int Y_Exec(char *filename, char *argv[])
{
    /**
     * Move args into kernel heap, so they aren't lost
     * For each page in our page table:
     *      Add the corresponding frame back to the queue of free frames
     * LoadProgram with the filename
     * Add PCB to ready queue
     *
     */
}

int Y_Exit(int status)
{
    /**
     * 1. Return all of USERLAND to the free_frames queue
     * 2. Return all of KERNAL_STACK to the free_frames queue
     * 3. Add this process to the zombie queue of parent if not null (dead)
     * 4. Save exit status in PCB
     * 6. Wake all processes waiting on the current process (in the waiters queue in the pcb), moving them into ready queue in kernal
     * 7. if initial process, HALT
     * 8. Pop next process off the ready queue (or whatever is ready to run - Zephyr said to run "scheduler"), run that
     */
}

int Y_Wait(int *status)
{
    /**
     * 1. Loop through children, find one that is dead. If no children, ERROR, else
     *        add to waiting queue and load new ready.
     * 2. Collect exit status from PCB, save to status and PID
     * 3. return PID.
     */
}

int Y_Getpid()
{
    /**
     * Using curr_process variable in kernel, get the pcb of the current process
     * Return the pid of the current process from this pcb
     */
    TracePrintf(1, "SYSCALL: Y_Getpid\n");
    return current_process->pid;
}

int Y_Brk(void *addr)
{
    /**
     * // Just a user brk
     * 1. Check if addr exceeds stack pointer, if so, ERROR
     * 2. Round addr to the next multiple of PAGESIZE bytes
     * 2. If addr is less than current brk, free all frames from addr to brk
     * 3. If addr is greater than current brk, allocate frames from brk to addr (inclusive) by doing the following:
     *      0. For each frame we need:
     *      a. If no more frames, ERROR
            b. Get first available frame from free_frames
     *          Increment brk by a page
     *          Map next page we need to this frame in the userland page table
     * 4. Set brk to addr
    */

    // return error if addr > stack pointer
    if (addr > current_process->user_c.sp)
    {
        return ERROR;
    }

    //set bottom of the red zone, which is the highest the brk can possibly be
    int bottom_of_red_zone_page = ((DOWN_TO_PAGE(current_process->user_c.sp)) - PAGESIZE) >> PAGESHIFT;

    // If addr below brk, free all frames from addr to brk
    if ((unsigned int) addr < current_process->brk)
    {
        for (int i = UP_TO_PAGE(addr) >> PAGESHIFT; i < current_process->brk >> PAGESHIFT; i++)
        {
                deallocateFrame(i);
        }
    }
    else if (((unsigned int)addr > current_process->brk))
    {
        for (int i = current_process->brk >> PAGESHIFT; i < UP_TO_PAGE(addr) >> PAGESHIFT; i++)
        {   
            // if we've gone too far and encroach on stack pointer, error
            if (i > bottom_of_red_zone_page){
                return ERROR;
            }
            int frame_index = allocateFrame();
            if (frame_index == -1)
            {
                return ERROR;
            }
            current_process->userland_pt[i].pfn = frame_index;
            current_process->userland_pt[i].prot = PROT_READ | PROT_WRITE;
            current_process->userland_pt[i].valid = 1;
        }
    }
    current_process->brk = UP_TO_PAGE(addr);
    return 0;
}


int Y_Delay(int num_ticks)
{
    /**
     * Get PCB of current process from kernel
     * Set ticks_delayed of this PCB to clock_ticks
     * Add this PCB to delayqueue
     * [further logic is handled by a function that's called by our OS upon receiving TRAP_CLOCK]
     */
    current_process->delayed_until = clock_ticks + num_ticks;
    enqueueDelayQueue(delay_queue, current_process);
    runProcess();
    return 0;
}

int Y_Ttyread(int tty_id, void *buf, int len)
{
    /**
     * While there is >= 1 node in the relevant user's terminal input buffer
     *     read the bytes from that node into buf, track how many bytes were read
     *     if buf is full or no more bytes to read
     *          exit loop
     * Return number of bytes read into buf
     *
     */
}

int Y_Ttywrite(int tty_id, void *buf, int len)
{
    /**
     * If address buf is not in kernel memory:
     *      Copy the contents of buf into kernel memory
     * Set can_transmit_to_terminal to false for this terminal
     * For each chunk of TERMINAL_MAX_LINE size in buf:
     *      call TTYtransmit to send this to the relevant terminal_output_buffer of the kernel
     *      wait for can_write_to_terminal to be true for this terminal
     */
}

int Y_Pipeinit(int *pipe_idp)
{
    /**
     * 1. Check if pipe_idp is valid, if not, ERROR
     * 2. Get first available pipe_id from free pipe queue
     * 3. Malloc pipe according to id, initialize read and write pos to 0, malloc buffer
     * 4. Fill kernel pipe array with pipe (maybe taken care of already by kernel?)
     * 5. Return 0
     */
}

int Y_Piperead(int pipe_id, void *buf, int len)
{
    /**
     * 1. Check if pipe_id is valid, if not, ERROR
     * 2. If len is greater than read-write pos, ERROR
     * 3. If no bytes to read, add current pcb to readers queue for this pipe
     * 4. Else:
     * 5.   Read in <=len bytes from read to min(read+len, read-write) into buf
     * 6.   Increment read_pos by len
     * 7.   Return len
     */
}

int Y_Pipewrite(int pipe_id, void *buf, int len)
{
    /**
     * 1. Check if pipe_id is valid, if not, ERROR
     * 2. If writing buffer from write_pos to end of buffer is less than len
     *     a. Copy from read-write pos to beginning of buffer
     *     b. reset read and write pos to 0 and len cur_buffer
     * 3. If writing buffer from write_pos still exceeds, ERROR
     * 4. Copy len bytes from buf to cur_buffer at write_pos
     * 5. Increment write_pos by len
     * 6. Move first pcb in the readers queue into the ready queue
     * 7. Return len
     *
     */
}

int Y_LockInit(int *lock_idp)
{
    /**
     * 1. Check if lock_idp is valid, if not, ERROR
     * 2. Get first available lock_id from free lock queue
     * 3. Malloc lock according to id, initialize waiting queue
     * 4. Fill kernel lock array with lock (maybe taken care of already by kernel?)
     * 5. Return 0
     */
}

int Y_Acquire(int lock_id)
{
    /**
     * 1. Check if lock_id is valid, if not, ERROR
     * 2. If lock is free, acquire it, mark it as acquired
     * 3. If lock is held, add caller to waiting queue of that lock
     * 4. Add pid to kernel waiting queue, switch to next ready process
     * 5. Return 0
     */
}

int Y_Release(int lock_id)
{
    /**
     * 1. Check if lock_id is valid, if not, ERROR
     * 2. If lock not held by this process, ERROR
     * 3. If lock is held by this process, release it, add to free queue
     * 4. unalloc memory
     * 5. Return 0
     */
}

int Y_CvarInit(int *cvar_idp)
{
    /**
     * 1. Check if cvar_idp is valid, if not, ERROR
     * 2. Get first available cvar_id from free cvar queue
     * 3. Malloc cvar according to id, initialize waiting queue
     * 4. Fill kernel cvar array with cvar (maybe taken care of already)
     * 5. Return 0
     */
}

int Y_CvarSignal(int cvar_id)
{
    /**
     * 1. Check if cvar_id is valid, if not, ERROR
     * 2. Move one process from waiting queue of cvar to ready queue of kernel
     * 3. Return 0
     */
}

int Y_CvarBroadcast(int cvar_id)
{
    /**
     * 1. Check if cvar_id is valid, if not, ERROR
     * 2. Move all processes from waiting queue of cvar to ready queue of kernel
     * 3. Return 0
     */
}

int Y_Cvarwait(int cvar_id, int lock_id)
{
    /**
     * 1. Check if lock_id, cvar_id is valid, if not, ERROR
     * 2. Try to release the lock, if it fails, ERROR, add to free queue if it succeeds
     * 3. Add this PID to the waiting queue of the cvar
     * 4. Switch to next ready process, add this process to waiting queue of kernel
     * 5. Reacquire the lock, return 0 and return to userland
     */
}

int Y_Reclaim(int id)
{
    /**
     * 1. Check if id is valid, if not, ERROR
     * 2. If id is a pipe, free all frames in the pipe's buffer
     * 3. If id is a lock, free the lock
     * 4. If id is a cvar, free the cvar
     * 5. Add id to free ____ IDs queue
     * 6. If anythign went wrong, ERROR or return 0 on success
     */
}
