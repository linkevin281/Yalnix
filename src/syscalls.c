/**
 * Kevin Lin and Carter Sullivan
 * Yalnix
 * CS58 - Operating Systems 24W
 *
 * syscalls.c
 *
 */

#include <ykernel.h>
#include "syscalls.h"
#include "kernel.h"

/**
 * Forks a new process.
 *
 * 1. Generates a new name {parent}_child for the child and creates a PCB.
 * 2. Updates parent's children to include the child, update child's parent to be the parent.
 * 3. Allocates a kernel stack for the child.
 * 4. Copies the parent's userland page table to the child's userland page table using a temp page below the kernel stack.
 * 5. Copies the parent's user context and other PCB fields to the child's PCB.
 * 6. Adds the parent and child to the ready queue.
 * 7. Calls KernelContextSwitch to copy the kernel context to the child, then runs the child.
 * 8. Returns 0 if the child, or the child's PID if the parent.
 */
int Y_Fork()
{
    TracePrintf(1, "SYSCALL: Y_Fork\n");

    // Generate Child Name
    char *name = malloc(sizeof(char) * 256);
    strncpy(name, current_process->name, 248);
    strcat(name, "_child");
    pcb_t *child = (pcb_t *)createPCB(name);
    TracePrintf(1, "Fork pt 1\n");
    if (child == NULL)
    {
        return ERROR;
    }

    // Update Parent and Child
    child->parent = current_process;
    enqueue(current_process->children, child);

    TracePrintf(1, "Fork pt 2\n");

    // Setup Kernel Stack
    for (int i = 0; i < KERNEL_STACK_MAXSIZE / PAGESIZE; i++)
    {
        int frame = allocateFrame(empty_frames);
        TracePrintf(1, "Allocating frame for kernel stack, frame: %d, mem: %p\n", frame, frame << PAGESHIFT);
        if (frame == -1)
        {
            TracePrintf(1, "Out of physical memory.\n");
            return ERROR;
        }
        child->kernel_stack_pt[i].pfn = frame;
        child->kernel_stack_pt[i].valid = 1;
        child->kernel_stack_pt[i].prot = PROT_READ | PROT_WRITE;
    }

    TracePrintf(1, "Fork pt 3\n");

    // Setup Region 1 Page Table
    for (int i = 0; i < MAX_PT_LEN; i++)
    {
        child->userland_pt[i].pfn = 0;
        child->userland_pt[i].valid = 0;
        child->userland_pt[i].prot = PROT_NONE;
    }

    int temp_base_page = (KERNEL_STACK_BASE - PAGESIZE) >> PAGESHIFT;
    kernel_pt[temp_base_page].valid = 1;
    kernel_pt[temp_base_page].prot = PROT_READ | PROT_WRITE;

    TracePrintf(1, "Fork pt 4\n");
    // Copy User PT from Parent
    for (int i = 0; i < MAX_PT_LEN; i++)
    {
        if (current_process->userland_pt[i].valid)
        {
            int frame = allocateFrame(empty_frames);
            //TracePrintf(1, "Fork pt 4.1\n");
            if (frame == -1)
            {
                TracePrintf(1, "frame is -1\n");
                return ERROR;
            }
            //TracePrintf(1, "Fork pt 4.2\n");
            child->userland_pt[i].pfn = frame;
            child->userland_pt[i].valid = 1;
            child->userland_pt[i].prot = current_process->userland_pt[i].prot;
            kernel_pt[temp_base_page].pfn = frame;
            WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);
            TracePrintf(1, "Allocated frame %d for page %d\n", frame, i);

            // Copy Mem at page i (add VMEM_0_SIZE to get to userland) to new frame
            memcpy((void *)(temp_base_page << PAGESHIFT), (void *)(i << PAGESHIFT) + VMEM_0_SIZE, PAGESIZE);

            //TracePrintf(1, "Copied page %d to frame %d\n", i, frame);
        }
    }
    TracePrintf(1, "Fork pt 5\n");
    TracePrintf(1, "Finished copying userland PT\n");
    kernel_pt[temp_base_page].valid = 0;

    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);

    // Copy User Context
    memcpy(&child->user_c, &current_process->user_c, sizeof(UserContext));

    // Copy other PCB fields
    child->delayed_until = current_process->delayed_until;
    child->state = current_process->state;
    // TODO:
    // Copy zombies
    // Copy waiters
    // Copy brk

    // Add to Ready Queue
    enqueue(ready_queue, child);

    // Copy Kernel Context
    if (KernelContextSwitch(KCCopy, child, NULL) == ERROR)
    {
        return ERROR;
    }
    
    // If we're the child, return 0
    if (getSize(current_process->children) == 0)
    {
        return 0;
    }
    else
    {
        // If we're the parent, return the child's PID
        return child->pid;
    }
}

/**
 * Executes a new process.
 * 
 * 1. Save Args and Filename in Kernel Heap
 * 2. Deallocate Stack Frames
 * 3. Create PCB
 * 4. Setup Kernel Stack
 * 5. Setup Region 1 Page Table
 * 6. Copy Kernel Context (copies stack)
 * 7. Load Program
*/
int Y_Exec(char *filename, char *argv[])
{
    TracePrintf(1, "SYSCALL: Y_Exec\n");
    char **args_copy = malloc(sizeof(char *) * MAX_ARGS);
    char *name_copy = malloc(sizeof(char) * MAX_ARG_LEN);
    strncpy(name_copy, filename, MAX_ARG_LEN);

    // Save Args in Kernel Heap
    if (argv == NULL)
    {
        TracePrintf(1, "Argv is null\n");
    }
    else
    {
        for (int i = 0; i < MAX_ARGS; i++)
        {
            if (argv[i] == NULL)
            {
                TracePrintf(1, "Argv[%d] is null\n", i);
                break;
            }
            args_copy[i] = malloc(sizeof(char) * MAX_ARG_LEN);
            strncpy(args_copy[i], argv[i], MAX_ARG_LEN);
        }
    }

    // Deallocate Stack Frames
    TracePrintf(1, "Deallocating stack frames\n");
    for (int i = 0; i < MAX_PT_LEN; i++)
    {
        pte_t cur_page = current_process->userland_pt[i];
        if (cur_page.valid)
        {
            deallocateFrame(cur_page.pfn);
        }
    }

    // Create PCB
    pcb_t *pcb = (pcb_t *)createPCB(filename);
    if (pcb == NULL)
    {
        return ERROR;
    }

    // Setup Kernel Stack
    for (int i = 0; i < KERNEL_STACK_MAXSIZE / PAGESIZE; i++)
    {
        int frame = allocateFrame(empty_frames);
        TracePrintf(1, "Allocating frame for kernel stack, frame: %d, mem: %p\n", frame, frame << PAGESHIFT);
        if (frame == -1)
        {
            TracePrintf(1, "Out of physical memory.\n");
            return ERROR;
        }
        pcb->kernel_stack_pt[i].pfn = frame;
        pcb->kernel_stack_pt[i].valid = 1;
        pcb->kernel_stack_pt[i].prot = PROT_READ | PROT_WRITE;
    }

    // Setup Region 1 Page Table
    for (int i = 0; i < MAX_PT_LEN; i++)
    {
        pcb->userland_pt[i].pfn = 0;
        pcb->userland_pt[i].valid = 0;
        pcb->userland_pt[i].prot = PROT_NONE;
    }

    KernelContextSwitch(KCCopy, pcb, NULL);
    LoadProgram(name_copy, args_copy, current_process);

    return 0;
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

    TracePrintf(1, "SYSCALL: Y_Exit\n");

    // return all of userland to free frames queue
    int frame_to_free;
    for (int i = 0; i < MAX_PT_LEN; i++)
    {
        int curr_pfn = current_process->userland_pt[i].pfn;
        // if the curr pfn is an actual frame
        if (curr_pfn >= 0 && curr_pfn < (int)(pmem_size_holder / PAGESIZE))
        {
            deallocateFrame(curr_pfn);
        }
    }

    TracePrintf(1, "in exit syscall, point 2\n");

    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);

    // return kernel stack frames to free frames queue
    for (int i = 0; i < KERNEL_STACK_MAXSIZE / PAGESIZE; i++)
    {
        int curr_pfn = current_process->kernel_stack_pt[i].pfn;
        // if the curr pfn is an actual frame
        if (curr_pfn >= 0 && curr_pfn < (int)(pmem_size_holder / PAGESIZE))
        {
            deallocateFrame(curr_pfn);
        }
    }

    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_KSTACK);

    TracePrintf(1, "in exit syscall, point 3\n");
    // add to queue of zombies for parent
    enqueue(current_process->parent->zombies, current_process);

    TracePrintf(1, "in exit syscall, point 4\n");
    current_process->state = DEAD;
    current_process->exit_status = status;
    
    // wake parent, if waiting on this process, and add to the ready queue
    if(current_process->parent != NULL){
            if(current_process->parent->state == WAITING){
                TracePrintf(1, "SEARCH for parent!\n");
                // traverse waiting queue to remove the node
                Node_t* prev = waiting_queue->head;
                Node_t* curr = waiting_queue->head->next;
                while(curr !=  waiting_queue->tail){
                    if(curr->data == current_process->parent){
                        prev->next = curr->next;
                        break;
                    }
                    prev = curr;
                    curr = curr->next;
                }
                current_process->parent->state = READY;
                TracePrintf(1, "TARGET2: \n");
                enqueue(ready_queue, current_process->parent);
            }
        }
    // no further scheduling logic needed, we can run the next process
    runProcess();
}

int Y_Wait(int *status)
{

    TracePrintf(1, "In wait syscall!\n");
    /**
     * 1. Loop through children, find one that is dead. If no children, ERROR, else
     *        add to waiting queue and load new ready.
     * 2. Collect exit status from PCB, save to status and PID
     * 3. return PID.
     */

    // return error if no dead children
    if (getSize(current_process->children) == 0 && getSize(current_process->zombies) == 0) {
        TracePrintf(1, "WAIT error\n");
        return ERROR;
    }

    else if (getSize(current_process->zombies) > 0){
        TracePrintf(1, "WAIT zombies in queue!\n");
        Node_t* child_container = dequeue(current_process->zombies);
        pcb_t* child = child_container->data;
        if(status != NULL){
            memcpy(status, &(child->exit_status), sizeof(int));
        }
        return child->pid;
    }

    current_process->state = WAITING;
    enqueue(waiting_queue, current_process);
    // immediately run the next process

    TracePrintf(1, "WAIT about to run process\n");
    // this will add the current process to the waiting queue
    runProcess();
    TracePrintf(1, "WAIT run process complete!\n");

    // at this point, there will be a zombie
    TracePrintf(1, "size of zombies queue: %d\n", getSize(current_process->zombies));
    Node_t* child_container = dequeue(current_process->zombies);
    TracePrintf(1, "WAIT checkpoint 1\n");
    pcb_t* child = child_container->data;
    TracePrintf(1, "TAGET child's pid: %d\n", child->pid);
    TracePrintf(1, "WAIT checkpoint 2\n");
    if(status != NULL){
        memcpy(status, &(child->exit_status), sizeof(int));
    }
    TracePrintf(1, "the child is done let's gooooo!\n");
    return child->pid;
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
    TracePrintf(1, "SYSCALL: Y_Brk\n");
    TracePrintf(1, "addr is %p, our brk: %p\n", addr, current_process->brk);
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
    TracePrintf(1, "highest text addr: %p\n", current_process->highest_text_addr);
    // return error if addr > stack pointer
    if ((unsigned int) addr > DOWN_TO_PAGE(current_process->user_c.sp))
    {
        TracePrintf(1, "attempting to allocate too much!\n");
        return ERROR;
    }
    // return error if addr is too low
    if((unsigned int) addr < current_process->highest_text_addr){
        TracePrintf(1, "addr too lowwwwww\n");
        return ERROR;
    }

    TracePrintf(1, "in brk, past error checks\n");

    int adjusted_addr_page = (UP_TO_PAGE(addr) - VMEM_0_SIZE) / PAGESIZE;
    TracePrintf(1, "Adjusted address page: %d\n", adjusted_addr_page);

    //set bottom of the red zone, which is the highest the brk can possibly be
    int bottom_of_red_zone_page = ((DOWN_TO_PAGE(current_process->user_c.sp)) - PAGESIZE) >> PAGESHIFT;

    TracePrintf(1, "bottom of red zone page: %d\n", bottom_of_red_zone_page);
    
    TracePrintf(1, "Is currend addr %p less than brk %p\n", addr, current_process->brk);
    // If addr below brk, free all frames from addr to brk
    if ((unsigned int)addr < current_process->brk)
    {
        for (int i = adjusted_addr_page; i < current_process->brk >> PAGESHIFT; i++)
        {
            deallocateFrame(i);
        }
    }
    else if (((unsigned int)addr > current_process->brk))
    {
        TracePrintf(1, "IN BRK: addr is greater than current brk\n");
        TracePrintf(1, "The loop: i starts at %d and goes to %d\n", current_process->brk >> PAGESHIFT, adjusted_addr_page);
        for (int i = current_process->brk >> PAGESHIFT; i < adjusted_addr_page; i++)
        {   
            // if we've gone too far and encroach on stack pointer, error
            if (i > bottom_of_red_zone_page)
            {
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
            TracePrintf(1, "BRK: just allocated frame %d for page %d\n", frame_index, i);
        }
    }
    current_process->brk = UP_TO_PAGE(addr) - VMEM_0_SIZE;
    TracePrintf(1, "IN BRK current process brk as address now %p\n", current_process->brk);
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
    TracePrintf(1, "SYSCALL: Y_Delay\n");
    current_process->delayed_until = clock_ticks + num_ticks;
    current_process->state = DELAYED;
    enqueueDelayQueue(delay_queue, current_process);
    TracePrintf(1, "SYSCALL: Y_Delay: Process name: %s is in state: %d\n", current_process->name, current_process->state);
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

    enqueue(want_to_read_from[tty_id], current_process); 


    Queue_t* input_queue = terminal_input_buffers[tty_id];
    int bytes_read = 0;
    Node_t* curr_node = input_queue->tail->prev;

    // while there is no input available or another node is ahead of us in the read queue, we run other process
    while(curr_node == input_queue->head || peekTail(want_to_read_from[tty_id])->data != current_process){
        // TODO: clean up this logic for queueing 
        current_process->state = READY;
        enqueue(ready_queue, current_process);
        runProcess();
        curr_node = input_queue->tail->prev;
    }  

    current_process->state = RUNNING;

    // while we can read more bytes or have read all bytes
    while(bytes_read < len && curr_node != input_queue->head){
    int curr_len = strlen(curr_node->data);
    if(curr_len + bytes_read < len){
            memcpy(buf, curr_node->data, curr_len);
            bytes_read+= curr_len;
            dequeue(input_queue);
            curr_node = input_queue->tail->prev;
        }
        else if (curr_len + bytes_read > len){
            memcpy(buf, curr_node->data, len - bytes_read);
            curr_node->data += len - bytes_read;
            bytes_read += len - bytes_read;
        }
        else{
            memcpy(buf, curr_node->data, curr_len);
            dequeue(input_queue);
        }
    }

    dequeue(want_to_read_from[tty_id]);
    return bytes_read;
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

    enqueue(want_to_write_to[tty_id], current_process);

    // allocate space in kernel to store the string, so it doesn't get lost when we switch processes
    void* kernel_buff = malloc(len);
    memcpy(kernel_buff, buf, len);

    // if there's another process in line to write to this terminal, block
    while(peekTail(want_to_write_to[tty_id])->data != current_process){
        // TODO: clean up this scheduling logic
        current_process->state = READY;
        enqueue(ready_queue, current_process);
        runProcess();
    }

    current_process->state = RUNNING;

    int bytes_read = 0;

    while(bytes_read < len && peekTail(want_to_write_to[tty_id])->data == current_process){
        if(can_write_to_terminal[tty_id] == 0){
            // TODO: clean up this scheduling logic
            current_process->state = READY;
            enqueue(ready_queue, current_process);
            runProcess();
            continue;
        }
        if(bytes_read + TERMINAL_MAX_LINE < len){
            TtyTransmit(tty_id, kernel_buff, TERMINAL_MAX_LINE);
            can_write_to_terminal[tty_id] = 0;
            kernel_buff += TERMINAL_MAX_LINE;
            bytes_read += TERMINAL_MAX_LINE;
        }
        else{
            TtyTransmit(tty_id, kernel_buff, len - bytes_read);
            can_write_to_terminal[tty_id] = 0;
            bytes_read = len;
        }
        // TODO: clean up this scheduling logic
        current_process->state = READY;
        enqueue(ready_queue, current_process);
        runProcess();
    }

    dequeue(want_to_write_to[tty_id]);

    return bytes_read == len ? len : ERROR;
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
