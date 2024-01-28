/**
 * Kevin Lin and Carter Sullivan
 * Yalnix
 * CS58 - Operating Systems 24W
 *
 * syscalls.c
 *
 */


// input: pcb
int Y_Fork()
{
 /**
  * Create new PCB for child process, with input PCB as its parent and user context copied in from input PCB
  * Update parent PCB to say that this new PCB is its child
  * Initialize a new page table for the child process
  * For each page in the page table of the parent process:
  *     If page is valid:
  *         Grab a free frame from empty_frames
  *         allocate this free frame to the page
  *         Copy contents of the page in the old table to the child
  * 
 */

}

int Y_Exec(char *filename, char *argv[])
{
    /**
     * Move args into kernel heap, so they aren't lost
     * For each page in our page table:
     *      Add the corresponding frame back to the queue of free frames
     * Move code into memory by doing the following (likely in another function):
     *  While there are bytes to be read from the file:
     *      Get first available frame from free_frames:
     *          Increment brk by a page
     *          Map next page in userland address space to this frame in the userland page table
     *          Read a page-sized chunk of bytes from the file, into this frame
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
     * 5. Loop thru children and set their parent (cur) to null
     * 6. Wake parent, move from waiting to ready queue in kernal
     * 7. if initial process, HALT
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
}

int Y_Brk(void *addr)
{
    /**
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
}

int Y_Delay(int clock_ticks)
{
}

int Y_Ttyread(int tty_id, void *buf, int len)
{
}

int Y_Ttywrite(int tty_id, void *buf, int len)
{
}

int Y_Pipeinit(int *pipe_idp)
{
}

int Y_Piperead(int pipe_id, void *buf, int len)
{
}

int Y_Pipewrite(int pipe_id, void *buf, int len)
{
}

int Y_LockInit(int *lock_idp)
{
}

int Y_Acquire(int lock_id)
{
}

int Y_Release(int lock_id)
{
}

int Y_CvarInit(int *cvar_idp)
{
}

int Y_CvarSignal(int cvar_id)
{
}

int Y_CvarBroadcast(int cvar_id)
{
}

int Y_Cvarwait(int cvar_id, int lock_id)
{
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
