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
     * 6. Wake parent if needed? 
     * 7. if initial process, HALT
    */
}

int Y_Wait(int *status)
{
}

int Y_Getpid()
{
}

int Y_Brk(void *addr)
{
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
}
