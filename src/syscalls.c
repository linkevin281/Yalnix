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
    /**
     * Get PCB of current process from kernel
     * Set ticks_delayed of this PCB to clock_ticks
     * Add this PCB to delayqueue
     * [further logic needs to be handled by a function that's called by our OS upon receiving TRAP_CLOCK]
    */
}

int Y_Ttyread(int tty_id, void *buf, int len)
{
    /**
     * We assume that first, TRAP_TTY_RECEIVE fires, and one of our functions reads user input into the relevant terminal_input_buffer.
     * Allocate string str on kernel heap
     * while len == TERMINAL_MAX_LINE:
     *      read buf into str
     *      wait for more input
     * for each byte up to len:
     *      read that byte into str
     * Create a new PCB for the terminal input, add it to the ready queue
     *      
    */
}

int Y_Ttywrite(int tty_id, void *buf, int len)
{
    /**
     * If address buf is not in kernel memory:
     *      Copy the contents of buf into kernel memory
     * For each chunk of TERMINAL_MAX_LINE size in buf:
     *      call TTYtransmit to send this to the relevant terminal_output_buffer of the kernel
     * Add to back of queue of waiting processes
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
     * 3. Read in <=len bytes from read to min(read+len, read-write) into buf
     * 4. Increment read_pos by len
     * 5. Return len
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
     * 6. Return len
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
     * 2. If lock is free, acquire it, add to held queue
     * 3. If lock is held, add to waiting queue of that lock
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
