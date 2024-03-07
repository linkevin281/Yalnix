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

    // Check if we own any locks
    if (current_process->owned_locks->size > 0)
    {
        return ERROR;
    }

    // Generate Child Name
    char *name = malloc(sizeof(char) * 256);
    strncpy(name, current_process->name, 248);
    strcat(name, "_child");
    pcb_t *child = (pcb_t *)createPCB(name);
    free(name);
    TracePrintf(1, "Fork pt 1\n");
    if (child == NULL)
    {
        return ERROR;
    }

    // Update Parent and Child
    child->parent = current_process;
    enqueue(current_process->children, child);
    TracePrintf(1, "Memo location of child: %p, childPDI: %d\n", child, child->pid);
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

    // Copy User PT from Parent
    for (int i = 0; i < MAX_PT_LEN; i++)
    {
        if (current_process->userland_pt[i].valid)
        {
            int frame = allocateFrame(empty_frames);
            // TracePrintf(1, "Fork pt 4.1\n");
            if (frame == -1)
            {
                TracePrintf(1, "frame is -1\n");
                return ERROR;
            }
            // TracePrintf(1, "Fork pt 4.2\n");
            child->userland_pt[i].pfn = frame;
            child->userland_pt[i].valid = 1;
            child->userland_pt[i].prot = current_process->userland_pt[i].prot;
            kernel_pt[temp_base_page].pfn = frame;
            WriteRegister(REG_TLB_FLUSH, temp_base_page << PAGESHIFT);

            // Copy Mem at page i (add VMEM_0_SIZE to get to userland) to new frame
            memcpy((void *)(temp_base_page << PAGESHIFT), (void *)(i << PAGESHIFT) + VMEM_0_SIZE, PAGESIZE);

            // TracePrintf(1, "Copied page %d to frame %d\n", i, frame);
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
    // TODO:
    // Copy zombies
    child->brk = current_process->brk;

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
    if(is_readable_string(filename) == 0 || is_readable_buffer(*argv, MAX_ARG_LEN) == 0){
        TracePrintf(1, "ERROR: Invalid input, you lack the correct permissions to access this memory.\n");
        return ERROR;
    }
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
     * 1.1 If there are any locks inited by this process, release them
     * 2. Return all of KERNAL_STACK to the free_frames queue
     * 3. Add this process to the zombie queue of parent if not null (dead)
     * 4. Save exit status in PCB
     * 7. if initial process, HALT
     * 8. Pop next process off the ready queue (or whatever is ready to run - Zephyr said to run "scheduler"), run that
     */

    TracePrintf(1, "SYSCALL: Y_Exit\n");

    // if current process is init process, halt the system
    if (current_process == init_process)
        Halt();
    TracePrintf(1, "In exit, my pid is %d, my name is %s\n", current_process->pid, current_process->name);
    current_process->is_alive = 0;

    // Free all initalized locks and releaseLock removes them if they are held.
    Node_t *node;
    while ((node = dequeue(current_process->inited_locks)) != NULL)
    {
        Lock_t *lock = (Lock_t *)node->data;
        releaseLock(lock, current_process);
        free(node);
        enqueue(empty_locks, lock);
    }

    TracePrintf(1, "in exit syscall, point 3\n");
    // add to queue of zombies for parent
    if (current_process->parent != NULL)
    {
        enqueue(current_process->parent->zombies, current_process);
        TracePrintf(1, "Just added myself to my parent's zombies queue...\n");
        TracePrintf(1, "Removing myself from my parent's children queue...\n");
        removePCBNode(current_process->parent->children, current_process);
    }

    TracePrintf(1, "in exit syscall, point 4\n");
    current_process->exit_status = status;

    // wake parent, if waiting on this process, and add to the ready queue
    if (current_process->parent != NULL && current_process->parent->is_alive && current_process->parent->is_waiting)
    {
        enqueue(ready_queue, current_process->parent);
        current_process->parent->is_waiting = 0;
    }

    // Release all held locks and wake any processes on them. Wake parent, if waiting on this process, and add to the ready queue
    for (int i = 0; i < NUM_LOCKS; i++)
    {
        if (locks[i].owner_pcb != NULL && locks[i].owner_pcb->pid == current_process->pid)
        {
            TracePrintf(1, "Found a lock that is owned by a dead process. Pid %d, lockID: %d\n", current_process->pid, locks[i].lock_id);
            releaseLock(&locks[i], current_process);
        }
    }

    // free pipes that this process was involved with, as needed
    // idle process is used as a placeholder for pipe reader/writer with processes that have died
    Queue_t *curr_pipe_queue = current_process->pipes;
    Node_t *curr_pipe_node;

    while ((curr_pipe_node = dequeue(curr_pipe_queue)) != NULL)
    {
        Pipe_t *curr_pipe = (Pipe_t *)(curr_pipe_node->data);
        // if both reader and writer of the pipe are gone, we free the pipe
        if ((curr_pipe->reader == current_process && curr_pipe->writer == idle_process) ||
            (curr_pipe->writer == current_process && curr_pipe->reader == idle_process))
        {
            // clean the relevant pipe
            TracePrintf(1, "KILLING pipe %d", curr_pipe->id);
            int i = curr_pipe->id;
            Y_Reclaim(i);
        }

        else if (curr_pipe->reader == current_process)
        {
            curr_pipe->reader = idle_process;
        }
        else if (curr_pipe->writer == current_process)
        {
            curr_pipe->writer = idle_process;
        }
        free(curr_pipe_node);
    }

    // tell children they've been orphaned
    Queue_t *kids = current_process->children;
    TracePrintf(1, "size kids queue: %d\n", getSize(kids));
    while (getSize(kids) > 0)
    {
        Node_t *curr = dequeue(kids);
        pcb_t *curr_pcb = curr->data;
        TracePrintf(1, "Child memory address: %p\n", curr_pcb);
        TracePrintf(1, "Just orphaned child %d from parent %d\n", curr_pcb->pid, current_process->pid);
        TracePrintf(1, "Currpcb parent: %d\n", curr_pcb->parent->pid);
        curr_pcb->parent = NULL;
    }

    deleteQueue(current_process->children);
    deleteQueue(current_process->zombies);
    deleteQueue(current_process->pipes);
    deleteQueue(current_process->inited_locks);
    deleteQueue(current_process->owned_locks);

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

    if (current_process->parent == NULL)
    {
        free(current_process);
    }

    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_KSTACK);

    // Section 3.4
    switch (status)
    {
    case ILLEGAL_INSTRUCTION:
        current_process->exit_status = ERROR;
        TracePrintf(0, "Illegal instruction. PID: %d\n", current_process->pid);
        break;
    case ILLEGAL_MEM_ADDR:
        current_process->exit_status = ERROR;
        TracePrintf(0, "Illegal memory address. Invalid permissions, address is not in userland, or address is too close to heap. PID: %d\n", current_process->pid);
        break;
    case EXECPTION_OUT_OF_MEM:
        current_process->exit_status = ERROR;
        TracePrintf(0, "Out of memory. PID: %d\n", current_process->pid);
        break;
    case FLOATING_POINT_EXCEPTION:
        current_process->exit_status = ERROR;
        TracePrintf(0, "Floating point exception. PID: %d\n", current_process->pid);
        break;
    case GENERAL_EXCEPTION:
        current_process->exit_status = ERROR;
        TracePrintf(0, "General exception. PID: %d\n", current_process->pid);
        break;
    default:
        break;
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
    if(is_writable_buffer((char*) status, 1) == 0){
        TracePrintf(1, "ERROR: Invalid input, you lack the correct permissions to access this memory.\n");
        return ERROR;
    }
    if (getSize(current_process->children) == 0 && getSize(current_process->zombies) == 0)
    {
        TracePrintf(1, "WAIT error\n");
        return ERROR;
    }

    else if (getSize(current_process->zombies) > 0)
    {
        TracePrintf(1, "WAIT zombies in queue!\n");
        Node_t *child_container = dequeue(current_process->zombies);
        pcb_t *child = child_container->data;
        int child_pid = child->pid;
        if (status != NULL)
        {
            memcpy(status, &(child->exit_status), sizeof(int));
        }
        free(child_container);
        free(child);
        return child_pid;
    }

    // immediately run the next process

    TracePrintf(1, "WAIT about to run process\n");
    // this will add the current process to the waiting queue
    current_process->is_waiting = 1;
    while (getSize(current_process->zombies) < 1)
    {
        runProcess();
    }
    TracePrintf(1, "WAIT run process complete!\n");

    // at this point, there will be a zombie
    TracePrintf(1, "size of zombies queue: %d\n", getSize(current_process->zombies));
    Node_t *child_container = dequeue(current_process->zombies);
    TracePrintf(1, "WAIT checkpoint 1\n");
    pcb_t *child = child_container->data;
    int child_pid = child->pid;
    TracePrintf(1, "TAGET child's pid: %d\n", child->pid);
    TracePrintf(1, "WAIT checkpoint 2\n");
    if (status != NULL)
    {
        memcpy(status, &(child->exit_status), sizeof(int));
    }
    TracePrintf(1, "the child is done let's gooooo!\n");
    free(child_container);
    free(child);
    return child_pid;
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
    if ((unsigned int)addr > DOWN_TO_PAGE(current_process->user_c.sp))
    {
        TracePrintf(1, "attempting to allocate too much!\n");
        return ERROR;
    }
    // return error if addr is too low
    if ((unsigned int)addr < current_process->highest_text_addr)
    {
        TracePrintf(1, "addr too lowwwwww\n");
        return ERROR;
    }

    TracePrintf(1, "in brk, past error checks\n");

    int adjusted_addr_page = (UP_TO_PAGE(addr) - VMEM_0_SIZE) / PAGESIZE;
    TracePrintf(1, "Adjusted address page: %d\n", adjusted_addr_page);

    // set bottom of the red zone, which is the highest the brk can possibly be
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
    if (num_ticks < 0)
    {
        TracePrintf(1, "ERROR: number of ticks must be at least 0\n");
        return ERROR;
    }

    TracePrintf(1, "SYSCALL: Y_Delay\n");
    current_process->delayed_until = clock_ticks + num_ticks;
    enqueueDelayQueue(delay_queue, current_process);
    TracePrintf(1, "SYSCALL: Y_Delay: Process name: %s\n", current_process->name);
    runProcess();
    return 0;
}

int Y_Ttyread(int tty_id, void *buf, int len)
{
    if(is_writable_buffer((char*) buf, len) == 0){
        TracePrintf(1, "ERROR: Invalid input, you lack the correct permissions to access this memory.\n");
        return ERROR;
    }
    /**
     * While there is >= 1 node in the relevant user's terminal input buffer
     *     read the bytes from that node into buf, track how many bytes were read
     *     if buf is full or no more bytes to read
     *          exit loop
     * Return number of bytes read into buf
     *
     */

    enqueue(want_to_read_from[tty_id], current_process);

    Queue_t *input_queue = terminal_input_buffers[tty_id];
    int bytes_read = 0;
    Node_t *curr_node = input_queue->tail->prev;

    // while there is no input available or another node is ahead of us in the read queue, we run other process
    while (curr_node == input_queue->head || ((getSize(want_to_read_from[tty_id]) > 0) && peekTail(want_to_read_from[tty_id])->data != current_process))
    {
        enqueue(ready_queue, current_process);
        runProcess();
        curr_node = input_queue->tail->prev;
    }

    // while we can read more bytes or have read all bytes
    while (bytes_read < len && curr_node != input_queue->head)
    {
        int curr_len = strlen(curr_node->data);
        if (curr_len + bytes_read < len)
        {
            memcpy(buf, curr_node->data, curr_len);
            free(curr_node->data);
            bytes_read += curr_len;
            free(dequeue(input_queue));
            curr_node = input_queue->tail->prev;
        }
        else if (curr_len + bytes_read > len)
        {
            memcpy(buf, curr_node->data, len - bytes_read);
            curr_node->data += len - bytes_read;
            bytes_read += len - bytes_read;
        }
        else
        {
            memcpy(buf, curr_node->data, curr_len);
            free(curr_node->data);
            free(dequeue(input_queue));
        }
    }

    free(dequeue(want_to_read_from[tty_id]));
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

    if(is_readable_buffer(buf, len) == 0){
        TracePrintf(1, "ERROR: Invalid input, you lack the correct permissions to access this memory.\n");
        return ERROR;
    }

    enqueue(want_to_write_to[tty_id], current_process);

    // allocate space in kernel to store the string, so it doesn't get lost when we switch processes
    void *kernel_buff = malloc(len);
    memcpy(kernel_buff, buf, len);

    // if there's another process in line to write to this terminal, block
    while (getSize(want_to_write_to[tty_id]) > 0 && peekTail(want_to_write_to[tty_id])->data != current_process)
    {
        enqueue(ready_queue, current_process);
        runProcess();
    }

    int bytes_read = 0;

    while (bytes_read < len && peekTail(want_to_write_to[tty_id])->data == current_process)
    {
        if (can_write_to_terminal[tty_id] == 0)
        {
            enqueue(ready_queue, current_process);
            runProcess();
            continue;
        }
        if (bytes_read + TERMINAL_MAX_LINE < len)
        {
            TtyTransmit(tty_id, kernel_buff, TERMINAL_MAX_LINE);
            can_write_to_terminal[tty_id] = 0;
            kernel_buff += TERMINAL_MAX_LINE;
            bytes_read += TERMINAL_MAX_LINE;
        }
        else
        {
            TtyTransmit(tty_id, kernel_buff, len - bytes_read);
            can_write_to_terminal[tty_id] = 0;
            bytes_read = len;
        }
        enqueue(ready_queue, current_process);
        runProcess();
    }

    free(dequeue(want_to_write_to[tty_id]));
    free(kernel_buff);

    return bytes_read == len ? len : ERROR;
}

int Y_Pipeinit(int *pipe_idp)
{
    if(is_writable_buffer((char*) pipe_idp, 1) == 0){
        TracePrintf(1, "ERROR: Invalid input, you lack the correct permissions to access this memory.\n");
        return ERROR;
    }
    TracePrintf(1, "SYSCALL: Y_Pipeinit\n");
    /**
     * 1. Check if pipe_idp is valid, if not, ERROR
     * 2. Get first available pipe_id from free pipe queue
     * 3. Malloc pipe according to id, initialize read and write pos to 0, malloc buffer
     * 4. Fill kernel pipe array with pipe (maybe taken care of already by kernel?)
     * 5. Return 0
     */

    if (getSize(empty_pipes) == 0)
    {
        TracePrintf(1, "Error - all pipes in use!\n");
        return ERROR;
    }

    // get next available pipe
    Node_t *pipe_holder = dequeue(empty_pipes);
    int available_pipe_id = *((int *)pipe_holder->data);
    free(pipe_holder->data);
    free(pipe_holder);
    memcpy(pipe_idp, &available_pipe_id, sizeof(int));
    pipes[available_pipe_id].exists = 1;
    return available_pipe_id;
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
    if(is_writable_buffer(buf, len) == 0){
        TracePrintf(1, "ERROR: Invalid input, you lack the correct permissions to access this memory.\n");
        return ERROR;
    }
    Pipe_t *curr_pipe = &(pipes[pipe_id]);
    curr_pipe->reader = current_process;
    enqueue(current_process->pipes, curr_pipe);
    char *buf_holder = (char *)buf;
    if (curr_pipe->exists == 0)
    {
        return ERROR;
    }
    if (len > PIPE_BUFFER_LEN)
    {
        return ERROR;
    }
    int bytes_to_read = len > curr_pipe->num_bytes_in_pipe ? curr_pipe->num_bytes_in_pipe : len;
    TracePrintf(1, "bytes to read: %d\n", bytes_to_read);
    Queue_t *pipe_reading_queue = want_to_read_pipe[pipe_id];

    enqueue(pipe_reading_queue, current_process);
    // wait for bytes to read
    while (bytes_to_read < 1 || can_interact_with_pipe[pipe_id] == 0 || (getSize(pipe_reading_queue) > 0 && peekTail(pipe_reading_queue)->data != current_process))
    {
        TracePrintf(1, "In loop and bytes to read: %d\n", bytes_to_read);
        if (getSize(pipe_reading_queue) > 0 && peekTail(pipe_reading_queue)->data != current_process)
        {
            TracePrintf(1, "In loop and decided we aren't first in queue\n");
        }
        if (can_interact_with_pipe[pipe_id] == 0)
        {
            TracePrintf(1, "We just can't interact with pipe\n");
        }
        runProcess();
        bytes_to_read = len > curr_pipe->num_bytes_in_pipe ? curr_pipe->num_bytes_in_pipe : len;
    }

    int bytes_read = 0;
    int pos = 0;
    // make sure nothing else interacts with our pipe while we do the below
    can_interact_with_pipe[pipe_id] = 0;

    while (bytes_read < bytes_to_read)
    {
        TracePrintf(1, "bytes_read: %d\n pos: %d\n current char: %c\n", bytes_read, pos, curr_pipe->buffer[curr_pipe->read_pos]);
        buf_holder[pos] = curr_pipe->buffer[curr_pipe->read_pos];
        bytes_read++;
        curr_pipe->read_pos++;
        pos++;
        curr_pipe->num_bytes_in_pipe--;
        // wrap the reading position around if needed
        if (curr_pipe->read_pos >= PIPE_BUFFER_LEN)
            curr_pipe->read_pos = 0;
    }

    can_interact_with_pipe[pipe_id] = 1;
    dequeue(pipe_reading_queue);

    // if more processes want to read or write from the pipe, put them in the ready queue
    if (getSize(pipe_reading_queue) > 0)
    {
        Node_t *curr_node = dequeue(pipe_reading_queue);
        enqueue(ready_queue, (pcb_t *)curr_node->data);
        free(curr_node);
    }

    Queue_t *pipe_writing_queue = want_to_write_pipe[pipe_id];
    if (getSize(pipe_writing_queue) > 0)
    {
        Node_t *curr_node = dequeue(pipe_writing_queue);
        enqueue(ready_queue, (pcb_t *)curr_node->data);
        free(curr_node);
    }

    return bytes_read;
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
    if(is_readable_buffer(buf, len) == 0){
        TracePrintf(1, "ERROR: Invalid input, you lack the correct permissions to access this memory.\n");
        return ERROR;
    }
    Pipe_t *curr_pipe = &(pipes[pipe_id]);
    curr_pipe->writer = current_process;
    TracePrintf(1, "Size of pipes queue for current process: %d\n", getSize(current_process->pipes));
    enqueue(current_process->pipes, curr_pipe);
    char *buf_str = (char *)buf;

    if (curr_pipe->exists == 0)
    {
        return ERROR;
    }

    if (len > PIPE_BUFFER_LEN)
    {
        return ERROR;
    }

    Queue_t *pipe_writing_queue = want_to_write_pipe[pipe_id];

    enqueue(pipe_writing_queue, current_process);

    while ((getSize(pipe_writing_queue) > 0 && (peekTail(pipe_writing_queue)->data != current_process)) || can_interact_with_pipe[pipe_id] == 0)
    {
        runProcess();
    }

    int pos = 0;

    can_interact_with_pipe[pipe_id] = 0;
    while (pos < len)
    {
        TracePrintf(1, "In loop, pos: %d\n", pos);
        TracePrintf(1, "In loop, len: %d\n", len);
        TracePrintf(1, "In loop, writing char: %c\n", buf_str[pos]);
        curr_pipe->buffer[curr_pipe->write_pos] = buf_str[pos];
        TracePrintf(1, "In loop, char actually written: %c\n", curr_pipe->buffer[curr_pipe->write_pos]);
        curr_pipe->num_bytes_in_pipe++;
        TracePrintf(1, "In loop, num bytes in pipe is: %d\n", curr_pipe->num_bytes_in_pipe);
        pos++;
        curr_pipe->write_pos++;
        // wrap around as needed
        if (curr_pipe->write_pos >= PIPE_BUFFER_LEN)
            curr_pipe->write_pos = 0;
    }

    can_interact_with_pipe[pipe_id] = 1;
    free(dequeue(pipe_writing_queue));

    // if more processes want to read or write from the pipe, put them in the ready queue
    if (getSize(pipe_writing_queue) > 0)
    {
        Node_t *curr_node = dequeue(pipe_writing_queue);
        enqueue(ready_queue, (pcb_t *)curr_node->data);
        free(curr_node);
    }

    Queue_t *pipe_reading_queue = want_to_read_pipe[pipe_id];

    if (getSize(pipe_reading_queue) > 0)
    {
        Node_t *curr_node = dequeue(pipe_reading_queue);
        enqueue(ready_queue, (pcb_t *)curr_node->data);
        free(curr_node);
    }

    return pos;
}

/**
 * 1. Check if there is a free lock.
 * 2. If there is, get the first available lock_id from free locks queue
 * 3. Copy lock id into lock_idp
 * 4. Return lock_idp
 */
int Y_LockInit(int *lock_idp)
{
    TracePrintf(1, "SYSCALL: Y_LockInit\n");
    if(is_writable_buffer((char*) lock_idp, 1) == 0){
        TracePrintf(1, "ERROR: Invalid input, you lack the correct permissions to access this memory.\n");
        return ERROR;
    }
    TracePrintf(1, "Queue size: %d\n", empty_locks->size);
    Node_t *lock_node = dequeue(empty_locks);
    TracePrintf(1, "Lock node: %p\n", lock_node);
    if (lock_node == NULL)
    {
        TracePrintf(1, "No more locks available\n");
        return ERROR;
    }
    Lock_t *lock = lock_node->data;
    free(lock_node);
    enqueue(current_process->inited_locks, lock);
    memccpy(lock_idp, &lock->lock_id, sizeof(int), sizeof(int));
    return *lock_idp;
}

/**
 * 1. Check if lock_id is valid, if not, ERROR
 * 2. Acquire lock.
 * 3. Return
 */
int Y_Acquire(int lock_id)
{
    TracePrintf(1, "SYSCALL: Y_Acquire\n");
    int scaled_lock_id = lock_id - MAX_PIPES;
    if (scaled_lock_id >= NUM_LOCKS || scaled_lock_id < 0)
    {
        return ERROR;
    }
    return acquireLock(&locks[scaled_lock_id], current_process);
}

/**
 * 1. Check if lock_id is valid, if not, ERROR
 * 2. Release lock.
 * 3. Return
 */
int Y_Release(int lock_id)
{
    TracePrintf(1, "SYSCALL: Y_Release\n");
    TracePrintf(1, "Lock id: %d\n", lock_id);
    int scaled_lock_id = lock_id - MAX_PIPES;
    if (scaled_lock_id >= NUM_LOCKS || scaled_lock_id < 0)
    {
        return ERROR;
    }
    return releaseLock(&locks[scaled_lock_id], current_process);
}

/**
 * 1. Check if there is a free cvar.
 * 2. If there is, get the first available cvar_id from free cvars queue
 * 3. Copy cvar id into cvar_idp
 * 4. Return cvar_idp
 */
int Y_CvarInit(int *cvar_idp)
{
    if(is_writable_buffer((char*) cvar_idp, 1) == 0){
        TracePrintf(1, "ERROR: Invalid input, you lack the correct permissions to access this memory.\n");
        return ERROR;
    }
    TracePrintf(1, "SYSCALL: Y_CvarInit\n");
    Node_t *cvar_node = dequeue(empty_cvars);
    if (cvar_node == NULL)
    {
        return ERROR;
    }
    Cvar_t *cvar = cvar_node->data;
    free(cvar_node);
    memccpy(cvar_idp, &cvar->cvar_id, sizeof(int), sizeof(int));
    return *cvar_idp;
}

/**
 * 1. Check if cvar_id is valid, if not, ERROR
 * 2. Release one process from the waiting queue of the cvar
 * 3. Return 0
 */
int Y_CvarSignal(int cvar_id)
{
    int scaled_cvar_id = cvar_id - MAX_PIPES - NUM_LOCKS;
    TracePrintf(1, "SYSCALL: Y_CvarSignal\n");
    if (cvar_id >= NUM_CVARS || cvar_id < 0)
    {
        return ERROR;
    }
    return cSignal(&cvars[scaled_cvar_id], current_process);
}

/**
 * 1. Check if cvar_id is valid, if not, ERROR
 * 2. Release all processes from the waiting queue of the cvar
 * 3. Return 0
 */
int Y_CvarBroadcast(int cvar_id)
{
    TracePrintf(1, "SYSCALL: Y_CvarBroadcast\n");
    int scaled_cvar_id = cvar_id - MAX_PIPES - NUM_LOCKS;
    if (scaled_cvar_id >= NUM_CVARS || scaled_cvar_id < 0)
    {
        return ERROR;
    }
    return cBroadcast(&cvars[scaled_cvar_id], current_process);
}

/**
 * 1. Check if lock_id, cvar_id is valid, if not, ERROR
 * 2. Try to release the lock, if it fails, ERROR, add to free queue if it succeeds
 * 3. Add this PID to the waiting queue of the cvar
 * 4. Switch to next ready process, add this process to waiting queue of kernel
 * 5. When woken try to acquire the lock again, return if successful
 */
int Y_CvarWait(int cvar_id, int lock_id)
{
    int scaled_cvar_id = cvar_id - MAX_PIPES - NUM_LOCKS;
    TracePrintf(1, "SYSCALL: Y_Cvarwait\n");
    if (scaled_cvar_id >= NUM_CVARS || scaled_cvar_id < 0)
    {
        return ERROR;
    }
    int scaled_lock_id = lock_id - MAX_PIPES;
    if (scaled_lock_id >= NUM_LOCKS || scaled_lock_id < 0)
    {
        return ERROR;
    }
    return cWait(&cvars[scaled_cvar_id], &locks[scaled_lock_id], current_process, &current_process->user_c);
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
    TracePrintf(1, "SYSCALL: Y_Reclaim\n");
    if (id < 0 || id > MAX_PIPES + NUM_LOCKS + NUM_CVARS)
    {
        return ERROR;
    }
    if (id < MAX_PIPES)
    {
        int i = id;
        pipes[i].id = i;
        pipes[i].read_pos = 0;
        pipes[i].write_pos = 0;
        pipes[i].exists = 0;
        pipes[i].num_bytes_in_pipe = 0;
        pipes[i].reader = NULL;
        pipes[i].writer = NULL;
        int *to_enqueue = malloc(sizeof(int));
        memcpy(to_enqueue, &i, sizeof(int));
        enqueue(empty_pipes, to_enqueue);
        Queue_t *pq_1 = createQueue();
        Queue_t *pq_2 = createQueue();
        want_to_read_pipe[i] = pq_1;
        want_to_write_pipe[i] = pq_2;
        can_interact_with_pipe[i] = 1;
    }
    else if (id < MAX_PIPES + NUM_LOCKS)
    {
        Lock_t *lock = &locks[id - MAX_PIPES];
        lock->is_locked = 0;
        lock->owner_pcb = NULL;
        deleteQueue(lock->waiting);
        lock->waiting = createQueue();
        int *lock_id = malloc(sizeof(int));
        memcpy(lock_id, &id, sizeof(int));
        enqueue(empty_locks, lock_id);
    }
    else
    {
        Cvar_t *cvar = &cvars[id - MAX_PIPES - NUM_LOCKS];
        deleteQueue(cvar->waiting);
        cvar->waiting = createQueue();
        int *cvar_id = malloc(sizeof(int));
        memcpy(cvar_id, &id, sizeof(int));
        enqueue(empty_cvars, cvar_id);
    }
}

int Y_Custom0(void)
{
    TracePrintf(1, "SYSCALL: Y_Custom0\n");
    peekMultiPCB(ready_queue, 10);
    return 0;
}

/**
 * * * * * * * * * * *
 * HELPER FUNCTIONS *
 * * * * * * * * * *
 */

/**
 * Creates a lock with the given id. Defined as index in arr.
 */
Lock_t *createLock(int lock_id)
{
    Lock_t *lock = (Lock_t *)malloc(sizeof(Lock_t));
    if (lock == NULL)
    {
        return NULL;
    }
    lock->lock_id = lock_id;
    lock->owner_pcb = NULL;
    lock->is_locked = 0;
    lock->waiting = createQueue();
    return lock;
}

/**
 * Acquires the lock. If lock is free, lock just gets acquired, fill owner.
 * Otherwise, add self to waiting queue and switch context out.
 */
int acquireLock(Lock_t *lock, pcb_t *pcb)
{
    TracePrintf(1, "Acquirer pid: %d\n", pcb->pid);
    if (lock->is_locked == 0)
    {
        lock->is_locked = 1;
        lock->owner_pcb = pcb;
        int *lock_id = malloc(sizeof(int));
        memcpy(lock_id, &lock->lock_id, sizeof(int));
        enqueue(pcb->owned_locks, lock_id);
        TracePrintf(1, "Acquired lock %d\n", lock->lock_id);
        return SUCCESS;
    }
    else if (lock->owner_pcb->pid == pcb->pid)
    {
        TracePrintf(1, "Process already owns lock\n");
        return SUCCESS;
    }
    else
    {
        TracePrintf(1, "Trying to access a locked lock, putting myself PID: %d on the waiting queue\n", pcb->pid);
        enqueue(lock->waiting, pcb);
        runProcess();
        TracePrintf(1, "I (PID: %d) woke up from waiting for lock %d, acquired lock.\n", pcb->pid, lock->lock_id);
        lock->is_locked = 1;
        lock->owner_pcb = pcb;
        int *lock_id = malloc(sizeof(int));
        memcpy(lock_id, &lock->lock_id, sizeof(int));
        enqueue(pcb->owned_locks, lock_id);
        return SUCCESS;
    }
}

/**
 * Releases the lock. If waiting queue, remove one process and move to ready queue, update state.
 */
int releaseLock(Lock_t *lock, pcb_t *pcb)
{
    TracePrintf(1, "FUNCTION: releaseLock\n");
    TracePrintf(1, "Lock id: %d\n", lock->lock_id);
    if (lock->is_locked == 0)
    {
        TracePrintf(1, "Not locked\n");
        return ERROR;
    }
    else if (lock->owner_pcb->pid != pcb->pid)
    {
        TracePrintf(1, "Not owner\n");
        return ERROR;
    }
    if (lock->waiting->size == 0)
    {
        TracePrintf(1, "No waiting\n");
        lock->is_locked = 0;
        lock->owner_pcb = NULL;
    }
    else
    {
        Node_t *node = dequeue(lock->waiting);
        pcb_t *pcb = (pcb_t *)node->data;
        TracePrintf(1, "Found a waiter PID: %d, waking them up\n", pcb->pid);
        free(node);
        enqueue(ready_queue, pcb);
    }

    TracePrintf(1, "Removing this lock from owned locks. Trying to match lock id: %d\n", lock->lock_id);
    TracePrintf(1, "Owned locks size: %d\n", pcb->owned_locks->size);
    int remove = removeFrameNode(pcb->owned_locks, lock->lock_id);
    if (remove == ERROR)
    {
        TracePrintf(1, "Error removing lock from owned locks\n");
        return ERROR;
    }
    return SUCCESS;
}

/**
 * Creates a condition variable with the given id. Defined as index in arr.
 */
Cvar_t *createCvar(int cvar_id)
{
    Cvar_t *cvar = (Cvar_t *)malloc(sizeof(Cvar_t));
    if (cvar == NULL)
    {
        return NULL;
    }
    cvar->cvar_id = cvar_id;
    cvar->waiting = createQueue();
    return cvar;
}

/**
 * Waits on condition variable. Adds itself to waiting queue.
 *   Will release the lock, move one process off the waiting queue
 *   and then runProcess to the next process in the ready queue. When it wakes in here
 *   it will try to reacquire the lock. Once it has the lock, it will continue running.
 *   Also checks if this process owns the lock it is trying to release.
 */
int cWait(Cvar_t *cvar, Lock_t *lock, pcb_t *caller, UserContext *user_context)
{
    if (lock->owner_pcb->pid != caller->pid)
    {
        return ERROR;
    }
    if (enqueue(cvar->waiting, caller) == -1)
    {
        return ERROR;
    }
    if (releaseLock(lock, caller) == -1)
    {
        return ERROR;
    }
    runProcess();
    TracePrintf(1, "Woke up from waiting, trying to acquire lock\n");
    if (acquireLock(lock, caller) == -1)
    {
        return ERROR;
    }
    return SUCCESS;
}
/**
 * Signals the condition variable. Removes one process from the waiting queue and moves it to the ready queue.
 *   Sets state to be ready
 */
int cSignal(Cvar_t *cvar, pcb_t *caller)
{
    if (cvar->waiting->size == 0)
    {
        return SUCCESS;
    }
    Node_t *node = dequeue(cvar->waiting);
    pcb_t *pcb = (pcb_t *)node->data;
    free(node);
    enqueue(ready_queue, pcb);
    TracePrintf(1, "Signaled a process, PID: %d\n", pcb->pid);
    TracePrintf(1, "Put %s in the ready queue\n", pcb->name);
    return SUCCESS;
}

/**
 * Broadcasts the condition variable. Removes all processes from the waiting queue and moves them to the ready queue.
 */
int cBroadcast(Cvar_t *cvar, pcb_t *caller)
{
    TracePrintf(1, "Size of waiting queue: %d\n", cvar->waiting->size);
    while (cvar->waiting->size > 0)
    {
        Node_t *node = dequeue(cvar->waiting);
        pcb_t *pcb = (pcb_t *)node->data;
        TracePrintf(1, "Found pid %d in the waiting queue\n", pcb->pid);
        free(node);
        enqueue(ready_queue, pcb);
    }
    return SUCCESS;
}