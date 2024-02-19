/**
 * Kevin Lin and Carter Sullivan
 * Yalnix
 * CS58 - Operating Systems 24W
 *
 * kernel.h
 *
 */

#include "kernel.h"
#include "trap.h"
#include <yalnix.h>
#include <hardware.h>
#include "syscalls.h"

// LOAD PROGRAM TEMPLATE
#include <fcntl.h>
#include <unistd.h>
#include <ykernel.h>
#include <load_info.h>

/* Queues to help indicate which resources are available (by index). */
Queue_t* ready_queue;
Queue_t* waiting_queue;
Queue_t* delay_queue; // This will be sorted. 
Queue_t* empty_locks;
Queue_t* empty_cvars;
Queue_t* empty_pipes;
Queue_t* empty_frames; // to track free frames

// each entry represents a terminal, and stores a linked list of strings with MAX_TERMINAL_LENGTH length each
Queue_t terminal_input_buffers[NUM_TERMINALS];
Queue_t terminal_output_buffers[NUM_TERMINALS];

int can_transmit_to_terminal[NUM_TERMINALS];

Pipe_t pipes[MAX_PIPES];
Lock_t locks[MAX_LOCKS];
Cvar_t cvars[MAX_CVARS];

pte_t kernel_pt[MAX_PT_LEN];

void* interrupt_vector_tbl[TRAP_VECTOR_SIZE];

int kernel_brk = 0;

pcb_t *current_process;
pcb_t *idle_process;

// For delay and traps
int clock_ticks = 0;

void KernelStart(char *cmd_args[], unsigned int pmem_size,
                 UserContext *uctxt)
{
    /**
     * For each page in pmem_size
     *      allocate a frame, adding it to empty_frames
     * Initialize kernel_pt
     * for each page between first_kernel_text_page and orig_kernel_brk_page
     *      add an entry to kernel_pt mapping that page to its physical address
     *      set permissions, validity, etc. for that page
     * Load the addresses of the bottom and top of kernel_pt into REG_PTBR0 and REG_PTLR0
     * Initialize all trap vectors
     * Initialize region 1 pagetable for idle, pop one frame off empty_frames and add it to this table
     * Enable vMEM
     * Initialize all queues()
     * Create idlePCB, including:
     *      region 1 page table for idle
     *      kernel stack frames for idle
     *      usercontext for idle
     *      pid for idle
     * Set userContext PC of idle to point to simple doIdle function (as described in manual)
     * Set usercontext SP of idle to point to region 1 page table for idle
     * Run idle
     * For anything in cmd_args:
     *      Create init_PCB for this process, including:
     *          region 1 page table
     *          new frames for kernel stack
     *          usercontext
     *          a PID
     *      Call KCCopy to copy kernelcontext into init_PCB
     *      Copy current kernel stack into kernel stack frames in init_PCB
     * LoadProgram on the process in init_PCB, allowing the process to run
     *
     */

    TracePrintf(1, "KernelStart called: initial values \n pmem_size: %d\n _first_kernel_text_page: %d\n _first_kernel_data_page: %d\n _orig_kernel_brk_page: %d\n", pmem_size, _first_kernel_text_page, _first_kernel_data_page, _orig_kernel_brk_page);
    kernel_brk = _orig_kernel_brk_page << PAGESHIFT;
    TracePrintf(1, "KernelStart: kernel_brk: %p\n", kernel_brk);

    // 1. Create Frame Queue
    // incrementing backwards, so that lowest-addresed frames are at front of the queue
    empty_frames = createQueue();
    for (int i = 0; i < (int)(pmem_size / PAGESIZE); i++)
    {
        if (enqueueHead(empty_frames, &i, sizeof(int)) == -1)
        {
            TracePrintf(1, "Error: Could not enqueue frame %d into empty_frames\n", i);
        }
    }
    TracePrintf(1, "Empty frames queue created\n");
    // As stated by SWS in ed, stuff below first text page has validity 0
    for (int i = 0; i < _first_kernel_text_page; i++)
    {
        TracePrintf(1, "Marking frame %d as invalid, it's below first kernel text page\n", i);
        unsigned long pt_index = removeFrameNode(empty_frames, i);
        kernel_pt[pt_index].valid = 0;
    }
    // Optimization: we can improve efficiency by doing one pass through the linked list after this for-loop, and removing all nodes we wanna remove
    // Optimization can be extended to all uses of removeFramenode in a loop
    for (int j = _first_kernel_text_page; j < _first_kernel_data_page; j++)
    {
        TracePrintf(1, "Allocating frame for kernel text, frame: %d, mem: %p\n", j, j << PAGESHIFT);
        // allocate frame -> (frame number/index) -> add to kernel_pt
        unsigned long pt_index = removeFrameNode(empty_frames, j);
        // per Miles, when vmem is off that just means pages must map exactly to their corresponding frames
        kernel_pt[pt_index].pfn = pt_index;
        kernel_pt[pt_index].prot = PROT_READ | PROT_EXEC;
        kernel_pt[pt_index].valid = 1;
    }

    // Optimization: we can improve efficiency by doing one pass through the linked list after this, and removing all nodes we wanna remove
    for (int k = _first_kernel_data_page; k < kernel_brk >> PAGESHIFT; k++)
    {
        TracePrintf(1, "Allocating frame for kernel data, frame: %d, mem: %p\n", k, k << PAGESHIFT);
        unsigned long pt_index = removeFrameNode(empty_frames, k);
        kernel_pt[pt_index].pfn = pt_index;
        kernel_pt[pt_index].prot = PROT_READ | PROT_WRITE;
        kernel_pt[pt_index].valid = 1;
    }

    TracePrintf(1, "Kernel PT initialized\n");
    WriteRegister(REG_PTBR0, (unsigned int)&kernel_pt);
    WriteRegister(REG_PTLR0, MAX_PT_LEN);

    // Initialize trap vectors
    interrupt_vector_tbl[TRAP_KERNEL] = TrapKernel;
    // interrupt_vector_tbl[TRAP_CLOCK] = TrapClock;
    interrupt_vector_tbl[TRAP_CLOCK] = Checkpoint3TrapClock;

    // interrupt_vector_tbl[TRAP_ILLEGAL] = TrapIllegal;
    // interrupt_vector_tbl[TRAP_MEMORY] = TrapMemory;
    // interrupt_vector_tbl[TRAP_MATH] = TrapMath;
    // interrupt_vector_tbl[TRAP_TTY_RECEIVE] = TrapTTYReceive;
    // interrupt_vector_tbl[TRAP_TTY_TRANSMIT] = TrapTTYTransmit;
    // interrupt_vector_tbl[TRAP_DISK] = TrapDisk;
    for (int i = TRAP_CLOCK + 1; i < TRAP_VECTOR_SIZE; i++)
    {
        interrupt_vector_tbl[i] = TrapElse;
    }
    // Set vector base
    WriteRegister(REG_VECTOR_BASE, (unsigned int)&interrupt_vector_tbl);

    // Final pt alloc in case brk grew
    int top_heap_page = UP_TO_PAGE(kernel_brk) >> PAGESHIFT;
    for (int l = _orig_kernel_brk_page + 1; l < top_heap_page; l++)
    {
        unsigned long pt_index = removeFrameNode(empty_frames, l);
        kernel_pt[pt_index].pfn = l;
        kernel_pt[pt_index].valid = 1;
        kernel_pt[pt_index].prot = PROT_READ | PROT_WRITE;
        TracePrintf(1, "Allocating frame for kernel heap, frame: %d, mem: %p\n", l, l << PAGESHIFT);
    }
    TracePrintf(1, "Idle process kernel stack init start\n");

    for (int i = KERNEL_STACK_BASE >> PAGESHIFT; i < KERNEL_STACK_LIMIT >> PAGESHIFT; i++)
    {
        TracePrintf(1, "Allocating frame for kernel stack, frame: %d, mem: %p\n", i, i << PAGESHIFT);
        int frame_index = removeFrameNode(empty_frames, i);
        kernel_pt[frame_index].pfn = frame_index;
        kernel_pt[frame_index].valid = 1;
        kernel_pt[frame_index].prot = PROT_READ | PROT_WRITE;
    }
    TracePrintf(1, "Idle process kernel stack init end\n");

    // Enable VMEM
    WriteRegister(REG_VM_ENABLE, 1);

    // Initialize queues

    ready_queue = createQueue();

    // Init and Idle Process
    char *idle_process_name = "./test/idle";
    char *init_process_name = "./test/init";

    pcb_t *init_process = initInitProcess(uctxt, cmd_args, init_process_name); // Calls KCCopy pid 0

    idle_process = initIdleProcess(uctxt, cmd_args, idle_process_name); // Joins after
    enqueue(ready_queue, init_process, sizeof(pcb_t));

    // if (cmd_args[0] != NULL)
    // {
    //     init_process_name = cmd_args[0];

    // }

    // Set uctxt to init so that the kernel knows to run
    init_process->state = RUNNING;
    current_process = idle_process;

    TracePrintf(1, "PIDS of idle and init: %d, %d\n", idle_process->pid, init_process->pid);

    uctxt->sp = current_process->user_c.sp;
    uctxt->pc = current_process->user_c.pc;
    // WriteRegister(REG_PTBR1, (unsigned int)(current_process->userland_pt));
    // WriteRegister(REG_PTLR1, MAX_PT_LEN);
    
    KernelContextSwitch(KCCopy, init_process, NULL);

    TracePrintf(1, "KernelStart: about to call KernelContextSwitch\n");
    // KernelContextSwitch(KCSwitch, NULL, idle_process);
}

KernelContext *KCSwitch(KernelContext *kc_in, void *curr_pcb_p, void *next_pcb_p)
{
    /**
     * - This function is called by KernelContextSwitch() in the hardware.
     * - Our kernel uses KernelContextSwitch() to switch between processes upon
     *      when the current process is done executing or some trap/wait other
     *      event occurs.
     *
     * • copy the current KernelContext into the old PCB
        • change the Region 0 kernel stack mappings to those for the new PCB
        • return a pointer to the KernelContext in the new PCB
     *
     */
    pcb_t *curr_pcb = (pcb_t *)curr_pcb_p;
    pcb_t *next_pcb = (pcb_t *)next_pcb_p;
    TracePrintf(1, "Pid of next process: %d, pid of current process: %d\n", next_pcb->pid, curr_pcb->pid);
    TracePrintf(1, "Kernel stack of curr: %d, %d\n", curr_pcb->kernel_stack_pt[0].pfn, curr_pcb->kernel_stack_pt[1].pfn);
    TracePrintf(1, "Kernel stack of next: %d, %d\n", next_pcb->kernel_stack_pt[0].pfn, next_pcb->kernel_stack_pt[1].pfn);

    // Copy KC into the current process's pcb
    memcpy(&curr_pcb->kernel_c, kc_in, sizeof(KernelContext));
    TracePrintf(1, "Kernel context copied\n");
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);

    // Copy kernel stack PT from kernel_pt to the current process's pcb
    for (int i = 0; i < KERNEL_STACK_MAXSIZE / PAGESIZE; i++)
    {
       
        TracePrintf(1, "Current kernel stack pfns, %d\n", kernel_pt[(KERNEL_STACK_BASE >> PAGESHIFT) + i].pfn);
        TracePrintf(1, "Next process kernel stack pfns, %d, %d\n", next_pcb->kernel_stack_pt[0].pfn, next_pcb->kernel_stack_pt[1].pfn);
        if (kernel_pt[(KERNEL_STACK_BASE >> PAGESHIFT) + i].valid != next_pcb->kernel_stack_pt[i].valid)
        {
            TracePrintf(1, "Kernel stack page %d is invalid\n", i);
        }
        if (kernel_pt[(KERNEL_STACK_BASE >> PAGESHIFT) + i].prot != next_pcb->kernel_stack_pt[i].prot)
        {
            TracePrintf(1, "Kernel stack page %d has different protection\n", i);
        }
        kernel_pt[(KERNEL_STACK_BASE >> PAGESHIFT) + i].pfn = next_pcb->kernel_stack_pt[i].pfn;
        kernel_pt[(KERNEL_STACK_BASE >> PAGESHIFT) + i].valid = next_pcb->kernel_stack_pt[i].valid;
        kernel_pt[(KERNEL_STACK_BASE >> PAGESHIFT) + i].prot = next_pcb->kernel_stack_pt[i].prot;
    }

    // Flush the TLB
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);
    
    TracePrintf(1, "Kernel stack copied\n");
    TracePrintf(1, "My kernel pfns: %d, %d\n", kernel_pt[(KERNEL_STACK_BASE >> PAGESHIFT)].pfn, kernel_pt[(KERNEL_STACK_BASE >> PAGESHIFT) + 1].pfn);
    current_process = next_pcb;

    WriteRegister(REG_PTBR1, (unsigned int)(current_process->userland_pt));
    WriteRegister(REG_PTLR1, MAX_PT_LEN);
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);

    // Return the next process's kernel context
    return &(next_pcb->kernel_c);
}
KernelContext *KCCopy(KernelContext *kc_in, void *new_pcb_p, void *not_used)
{
    /**
     * - KCCopy will copy kc_in into the new process's pcb
     *
     * 1. Copy KC into the new process's pcb
     * 2. Copy kernel stack PT into the new process's pcb
     * 3. Flush the TLB
     * 4. Return the new process's kernel context
     *
     */
    pcb_t *new_pcb = (pcb_t *)new_pcb_p;

    // Copy KC into the new process's pcb
    memcpy(&new_pcb->kernel_c, kc_in, sizeof(KernelContext));

    // temporary place to store frames in for loop below
    int temp_base_page = (KERNEL_STACK_BASE - PAGESIZE) >> PAGESHIFT;
    kernel_pt[temp_base_page].valid = 1;

    TracePrintf(1, "Temp page original pfn: %d\n", kernel_pt[temp_base_page].pfn);

    int first_page_in_kstack = (VMEM_0_SIZE - KERNEL_STACK_MAXSIZE) >> PAGESHIFT;

    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);

    // Copy kernel stack PT into  the new process's pcb
    for (int i = 0; i < KERNEL_STACK_MAXSIZE / PAGESIZE; i++)
    {
        // map page to same address as kernel stack page
        kernel_pt[temp_base_page].pfn = new_pcb->kernel_stack_pt[i].pfn;
        kernel_pt[temp_base_page].prot = new_pcb->kernel_stack_pt[i].prot;
        kernel_pt[temp_base_page].valid = new_pcb->kernel_stack_pt[i].valid;
        TracePrintf(1, "Just changed pfn of temp page to %d. Temp page lives at page %d\n", kernel_pt[temp_base_page].pfn, temp_base_page);
        WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);

        // copy ith page in stack into temp address
        memcpy((void *)(temp_base_page << PAGESHIFT), (void *)((first_page_in_kstack + i) << PAGESHIFT), PAGESIZE);
    
        TracePrintf(1, "Just copied page %d into temp page %d\n", first_page_in_kstack + i, temp_base_page);

        // kernel_pt[(KERNEL_STACK_BASE >> PAGESHIFT)+i].pfn = new_pcb->kernel_stack_pt[i].pfn;
    }
    // WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);

    TracePrintf(1, "Kernel stack copied\n");
    kernel_pt[temp_base_page].valid = 0;

    // write kernel stack into kernel pt
    // for (int i = 0; i < KERNEL_STACK_MAXSIZE / PAGESIZE; i++)
    // {
    //     kernel_pt[first_page_in_kstack + i].pfn = new_pcb->kernel_stack_pt[i].pfn;
    //     kernel_pt[first_page_in_kstack + i].valid = new_pcb->kernel_stack_pt[i].valid;
    //     kernel_pt[first_page_in_kstack + i].prot = new_pcb->kernel_stack_pt[i].prot;
    // }

    // Flush the TLB
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);

    return kc_in;
}

/**
 * If there are frames in empty_frames:
 *      pop the first frame off of empty_frames
 *      Return the number of this frame
 * Else:
 *      Return -1 (this could potentially be modified by evicting an existing frame - something to pursue once base functionality is working)
 */
int allocateFrame()
{
    if (getSize(empty_frames) > 0)
    {
        return *(int *)dequeue(empty_frames)->data;
    }
    else
    {
        return ERROR;
    }
}

/**
 * Returns the frame_index of the frame that was deallocated
 *
 */
int deallocateFrame(int frame_index)
{
    /**
     * Add a node with frame_index to empty_frames
     * NOTE: user is responsible for flushing TLB after calling!
     */
    if (enqueue(empty_frames, &frame_index, sizeof(int)) == -1)
    {
        return ERROR;
    }
    else
    {
        return frame_index;
    }
}

int runProcess()
{
    /**
     * Grab the first pcb off of the ready queue. This is the pcb to_switch
     * If current process has more instructions to run AND is in "running" state (indicating it has been running with no issue)
     *      add it to the ready queue
     * Else if current process in zombie state (indicating it is dead):
     *      Move it into its parent's queue of zombies
     * Call KernelContextSwitch(KCSwitch) with to_switch, to switch this process in
     * Change current_process to the pcb of to_switch
     */
    TracePrintf(1, "Queue size: %d\n", getSize(ready_queue));

    pcb_t *next = (pcb_t *)dequeue(ready_queue)->data;
    if (next == NULL)
    {
        TracePrintf(1, "No process to run\n");
        return ERROR;
    }
    if (current_process->state == RUNNING)
    {
        enqueue(ready_queue, current_process, sizeof(pcb_t));
    }
    else if (current_process->state == DEAD)
    {
        enqueue(current_process->parent->zombies, current_process, sizeof(pcb_t));
    }
    TracePrintf(1, "About to call KernelContextSwitch\n");

    TracePrintf(1, "PID OF cur: %d, PID OF next: %d\n", current_process->pid, next->pid);

    if (KernelContextSwitch(KCSwitch, current_process, next) == ERROR)
    {
        TracePrintf(1, "Error in KernelContextSwitch\n");
        return ERROR;
    }
    TracePrintf(1, "Back from KernelContextSwitch\n");
}

int SetKernelBrk(void *addr)
{
    /**
     * Check if addr exceeds kernel stack pointer, if so, ERROR
     * Round addr to next multiple of PAGESIZE bytes
     * If vmem is not enabled:
     *      If addr is less than current kernelBrk, free all frames from addr to kernelBrk
            Else If addr is greater than current kernelBrk, allocate frames from kernelBrk to addr (inclusive) by doing the following:
            *  0. For each frame we need, starting one above the frame for the current KernelBrk:
                    Do linear search in queue of empty frames for that frame
                    Allocate that frame for the process, add it to kernel_pt
    * If vmem is enabled:
        If addr is less than current kernelBrk, free all frames from addr to kernelBrk
     * If addr is greater than current kernelBrk, allocate frames from kernelBrk to addr (inclusive) by doing the following:
     *      0. For each frame we need:
     *      a. If no more frames, ERROR
            b. Get first available frame from free_frames
     *          Increment kernelBrk by a page
     *          Map next page we need to this frame in the userland page table
     * 4. Set kernelBrk to the page represented by addr
    */

    TracePrintf(1, "SetKernelBrk called with addr: %p\n", addr);

    // Bounds check: if address is less than start of data or above lowest point kernel stack can be, error
    if ((unsigned int)addr < _orig_kernel_brk_page << PAGESHIFT || (unsigned int)addr > (unsigned int)(KERNEL_STACK_LIMIT - KERNEL_STACK_MAXSIZE))
    {
        TracePrintf(1, "SetKernelBrk called with bad addr of %p", addr);
        return ERROR;
    }

    // If VMEM not enabled, we just move kernel_brk
    if (ReadRegister(REG_VM_ENABLE) != 1)
    {
        TracePrintf(1, "VMEM off, setting kernel brk to: %d\n", (unsigned int)addr);
        kernel_brk = (unsigned int)addr;
    }
    // VMEM is enabled
    else
    {
        // If addr is less than current kernelBrk, free all frames from addr to kernelBrk
        if ((unsigned int)addr < kernel_brk)
        {
            int bottom_page_index = UP_TO_PAGE(addr) >> PAGESHIFT;
            int top_page_index = kernel_brk >> PAGESHIFT;
            for (int i = bottom_page_index; i < top_page_index; i++)
            {
                deallocateFrame(i);
                kernel_pt[i].valid = 0;
            }
            kernel_brk = UP_TO_PAGE(addr);
        }
        // If addr is greater than current kernelBrk, allocate frames from kernelBrk to addr (inclusive)
        else if ((unsigned int)addr > kernel_brk)
        {
            int bottom_page_index = kernel_brk >> PAGESHIFT;
            int top_page_index = UP_TO_PAGE(addr) >> PAGESHIFT;
            TracePrintf(1, "bottom_page_index: %d, top_page_index: %d\n", bottom_page_index, top_page_index);
            TracePrintf(1, "ReadRegister(REG_VM_ENABLE): %d\n", ReadRegister(REG_VM_ENABLE));
            for (int i = bottom_page_index; i < top_page_index; i++)
            {
                int frame_index = allocateFrame();
                if (frame_index == -1)
                {
                    return ERROR;
                }
                kernel_pt[frame_index].pfn = frame_index;
                kernel_pt[frame_index].valid = 1;
                kernel_pt[frame_index].prot = PROT_READ | PROT_WRITE;
                TracePrintf(1, "Allocated frame %d for kernel_pt at memory address %p\n", frame_index, frame_index << PAGESHIFT);
            }
            kernel_brk = UP_TO_PAGE(addr);
            TracePrintf(1, "SetKernelBrk: kernel_brk: %p\n", kernel_brk);
        }
    }
    return 0;
}

pcb_t *initIdleProcess(UserContext *uctxt, char *args[], char *name)
{
    pcb_t *idle_process = createPCB();
    TracePrintf(1, "Idle process created %d\n", idle_process->pid);

    TracePrintf(1, "Idle process created\n");
    memcpy(&idle_process->user_c, uctxt, sizeof(UserContext)); // For after checkpoint 2

    WriteRegister(REG_PTBR1, (unsigned int)(idle_process->userland_pt));
    WriteRegister(REG_PTLR1, MAX_PT_LEN);
    // Setup Kernel Stack
    for (int i = 0; i < KERNEL_STACK_MAXSIZE / PAGESIZE; i++)
    {
        idle_process->kernel_stack_pt[i].pfn = kernel_pt[(KERNEL_STACK_BASE >> PAGESHIFT) + i].pfn;
        idle_process->kernel_stack_pt[i].valid = kernel_pt[(KERNEL_STACK_BASE >> PAGESHIFT) + i].valid;
        idle_process->kernel_stack_pt[i].prot = kernel_pt[(KERNEL_STACK_BASE >> PAGESHIFT) + i].prot;
    }

    // Init region 1 page table for idle process
    TracePrintf(1, "Idle process region 1 page table init start\n");
    for (int i = 0; i < MAX_PT_LEN; i++)
    {
        idle_process->userland_pt[i].pfn = 0;
        idle_process->userland_pt[i].pfn = 0;
        idle_process->userland_pt[i].valid = 0;
    }

    LoadProgram(name, args, idle_process);
    idle_process->state = READY;

    // WriteRegister(REG_PTBR1, (unsigned int)(idle_process->userland_pt));
    // WriteRegister(REG_PTLR1, MAX_PT_LEN);

    // Checkpoint 2 code DELETE LATER
    // Since we're 32-bit, we need to set the kernel stack pointer to 4 bytes below the top of the kernel stack
    // uctxt->sp = (void *)((page << PAGESHIFT) + PAGESIZE - 4);
    // uctxt->pc = (void *)DoIdle;
    // WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);

    // Checkpoint 3 code
    // current_process = idle_process;

    return idle_process;
}

pcb_t *initInitProcess(UserContext *uctxt, char *args[], char *name)
{

    pcb_t *init_process = createPCB();
    TracePrintf(1, "Init process created %d\n", init_process->pid);
    // copy user context passed in to our pcb
    memcpy(&init_process->user_c, uctxt, sizeof(UserContext));
    
    // for kernel stack
    int top_page = (VMEM_1_SIZE >> PAGESHIFT);
    for (int i = 0; i < KERNEL_STACK_MAXSIZE / PAGESIZE; i++)
    {
        int frame = allocateFrame(empty_frames);
        TracePrintf(1, "hereAllocating frame for kernel stack, frame: %d, mem: %p\n", frame, frame << PAGESHIFT);
        if (frame == -1)
        {
            TracePrintf(1, "Out of physical memory.\n");
            return NULL;
        }
        init_process->kernel_stack_pt[i].pfn = frame;
        init_process->kernel_stack_pt[i].valid = 1;
        init_process->kernel_stack_pt[i].prot = PROT_READ | PROT_WRITE;
    }

    // Init region 1 page table for init process
    TracePrintf(1, "Idle process region 1 page table init start\n");
    for (int i = 0; i < MAX_PT_LEN; i++)
    {
        init_process->userland_pt[i].pfn = 0;
        init_process->userland_pt[i].pfn = 0;
        init_process->userland_pt[i].valid = 0;
    }

    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);

    LoadProgram(name, args, init_process);
    // KernelContextSwitch(KCCopy, init_process, NULL);

    TracePrintf(1, "Stack p address: %p\n", init_process->user_c.sp);

    // TracePrintf(1, "About to clone idle into init\n");
    // KernelContextSwitch(KCCopy, init_process, NULL);
    // TracePrintf(0, "Back from the clone---am I idle or init?\n");

    return init_process;
}

pcb_t *createPCB()
{
    pcb_t *pcb = malloc(sizeof(pcb_t));
    pcb->pid = helper_new_pid(pcb->userland_pt);
    pcb->exit_status = 0;
    pcb->ticks_delayed = 0;
    pcb->state = RUNNING;
    pcb->children = createQueue();
    pcb->zombies = createQueue();
    pcb->waiters = createQueue();
    pcb->brk = 0;
    return pcb;
}

/*
 *  Load a program into an existing address space.  The program comes from
 *  the Linux file named "name", and its arguments come from the array at
 *  "args", which is in standard argv format.  The argument "proc" points
 *  to the process or PCB structure for the process into which the program
 *  is to be loaded.
 */
int LoadProgram(char *name, char *args[], pcb_t *proc)
{
    int fd;
    int (*entry)();
    struct load_info li;
    int i;
    char *cp;
    char **cpp;
    char *cp2;
    int argcount;
    int size;
    int text_pg1;
    int data_pg1;
    int data_npg;
    int stack_npg;
    long segment_size;
    char *argbuf;
    TracePrintf(1, "LoadProgram called with name: %s\n", name);
    /*
     * Open the executable file
     */
    if ((fd = open(name, O_RDONLY)) < 0)
    {
        TracePrintf(0, "LoadProgram: can't open file '%s'\n", name);
        return ERROR;
    }

    if (LoadInfo(fd, &li) != LI_NO_ERROR)
    {
        TracePrintf(0, "LoadProgram: '%s' not in Yalnix format\n", name);
        close(fd);
        return (-1);
    }

    if (li.entry < VMEM_1_BASE)
    {
        TracePrintf(0, "LoadProgram: '%s' not linked for Yalnix\n", name);
        close(fd);
        return ERROR;
    }

    /*
     * Figure out in what region 1 page the different program sections
     * start and end
     */
    text_pg1 = (li.t_vaddr - VMEM_1_BASE) >> PAGESHIFT;
    data_pg1 = (li.id_vaddr - VMEM_1_BASE) >> PAGESHIFT;
    data_npg = li.id_npg + li.ud_npg;
    /*
     *  Figure out how many bytes are needed to hold the arguments on
     *  the new stack that we are building.  Also count the number of
     *  arguments, to become the argc that the new "main" gets called with.
     */
    size = 0;
    for (i = 0; args[i] != NULL; i++)
    {
        TracePrintf(3, "counting arg %d = '%s'\n", i, args[i]);
        size += strlen(args[i]) + 1;
    }
    argcount = i;

    TracePrintf(2, "LoadProgram: argsize %d, argcount %d\n", size, argcount);

    /*
     *  The arguments will get copied starting at "cp", and the argv
     *  pointers to the arguments (and the argc value) will get built
     *  starting at "cpp".  The value for "cpp" is computed by subtracting
     *  off space for the number of arguments (plus 3, for the argc value,
     *  a NULL pointer terminating the argv pointers, and a NULL pointer
     *  terminating the envp pointers) times the size of each,
     *  and then rounding the value *down* to a double-word boundary.
     */
    cp = ((char *)VMEM_1_LIMIT) - size;

    cpp = (char **)(((int)cp -
                     ((argcount + 3 + POST_ARGV_NULL_SPACE) * sizeof(void *))) &
                    ~7);

    /*
     * Compute the new stack pointer, leaving INITIAL_STACK_FRAME_SIZE bytes
     * reserved above the stack pointer, before the arguments.
     */
    cp2 = (caddr_t)cpp - INITIAL_STACK_FRAME_SIZE;

    TracePrintf(1, "prog_size %d, text %d data %d bss %d pages\n",
                li.t_npg + data_npg, li.t_npg, li.id_npg, li.ud_npg);

    /*
     * Compute how many pages we need for the stack */
    stack_npg = (VMEM_1_LIMIT - DOWN_TO_PAGE(cp2)) >> PAGESHIFT;

    TracePrintf(1, "LoadProgram: heap_size %d, stack_size %d\n",
                li.t_npg + data_npg, stack_npg);

    /* leave at least one page between heap and stack */
    if (stack_npg + data_pg1 + data_npg >= MAX_PT_LEN)
    {
        close(fd);
        return ERROR;
    }

    /*
     * This completes all the checks before we proceed to actually load
     * the new program.  From this point on, we are committed to either
     * loading succesfully or killing the process.
     */

    /*
     * Set the new stack pointer value in the process's UserContext
     */
    proc->user_c.sp = cp2;

    /*
     * Now save the arguments in a separate buffer in region 0, since
     * we are about to blow away all of region 1.
     */
    cp2 = argbuf = (char *)malloc(size);

    if (cp2 == NULL)
    {
        TracePrintf(0, "LoadProgram: malloc failed\n");
        return ERROR;
    }
    // => Maybe check if valid flag on cp2 is 1?

    for (i = 0; args[i] != NULL; i++)
    {
        TracePrintf(3, "saving arg %d = '%s'\n", i, args[i]);
        strcpy(cp2, args[i]);
        cp2 += strlen(cp2) + 1;
    }

    /*
     * Set up the page tables for the process so that we can read the
     * program into memory.  Get the right number of physical pages
     * allocated, and set them all to writable.
     */

    /* ==>> Throw away the old region 1 virtual address space by
     * ==>> curent process by walking through the R1 page table and,
     * ==>> for every valid page, free the pfn and mark the page invalid.
     */
    for (int i = 0; i < MAX_PT_LEN; i++)
    {
        if (proc->userland_pt[i].valid == 1)
        {
            deallocateFrame(proc->userland_pt[i].pfn);
            proc->userland_pt[i].valid = 0;
        }
    }
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);

    /*
     * ==>> Then, build up the new region1.
     * ==>> (See the LoadProgram diagram in the manual.)
     */

    /*
     * ==>> Then, stack. Allocate "stack_npg" physical pages and map them to the top
     * ==>> of the region 1 virtual address space.
     * ==>> These pages should be marked valid, with a
     * ==>> protection of (PROT_READ | PROT_WRITE).
     */
    for (int i = MAX_PT_LEN - stack_npg; i < MAX_PT_LEN; i++)
    {
        int frame = allocateFrame();
        if (frame == -1)
        {
            TracePrintf(1, "Out of physical memory.\n");
            return ERROR;
        }
        proc->userland_pt[i].pfn = frame;
        proc->userland_pt[i].valid = 1;
        proc->userland_pt[i].prot = PROT_READ | PROT_WRITE;
    }

    /*
     * ==>> Then, data. Allocate "data_npg" physical pages and map them starting at
     * ==>> the  "data_pg1" in region 1 address space.
     * ==>> These pages should be marked valid, with a protection of
     * ==>> (PROT_READ | PROT_WRITE).
     */
    for (int i = data_pg1; i < data_pg1 + data_npg; i++)
    {
        int frame = allocateFrame();
        if (frame == -1)
        {
            TracePrintf(1, "Out of physical memory.\n");
            return ERROR;
        }
        proc->userland_pt[i].pfn = frame;
        proc->userland_pt[i].valid = 1;
        proc->userland_pt[i].prot = PROT_READ | PROT_WRITE;
    }

    /*
     * ==>> First, text. Allocate "li.t_npg" physical pages and map them starting at
     * ==>> the "text_pg1" page in region 1 address space.
     * ==>> These pages should be marked valid, with a protection of
     * ==>> (PROT_READ | PROT_WRITE).
     */
    for (int i = text_pg1; i < text_pg1 + li.t_npg; i++)
    {
        int frame = allocateFrame();
        if (frame == -1)
        {
            TracePrintf(1, "Out of physical memory.\n");
            return ERROR;
        }
        proc->userland_pt[i].pfn = frame;
        proc->userland_pt[i].valid = 1;
        proc->userland_pt[i].prot = PROT_READ | PROT_WRITE;
    }

    /*
     * ==>> (Finally, make sure that there are no stale region1 mappings left in the TLB!)
     */
    WriteRegister(REG_PTBR1, (unsigned int)(proc->userland_pt));
    WriteRegister(REG_PTLR1, MAX_PT_LEN);
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);

    /*
     * All pages for the new address space are now in the page table.
     */

    /*
     * Read the text from the file into memory.
     */
    lseek(fd, li.t_faddr, SEEK_SET);
    segment_size = li.t_npg << PAGESHIFT;
    if (read(fd, (void *)li.t_vaddr, segment_size) != segment_size)
    {
        close(fd);
        return KILL; // see ykernel.h
    }

    /*
     * Read the data from the file into memory.
     */
    lseek(fd, li.id_faddr, 0);
    segment_size = li.id_npg << PAGESHIFT;

    if (read(fd, (void *)li.id_vaddr, segment_size) != segment_size)
    {
        close(fd);
        return KILL;
    }
    close(fd); /* we've read it all now */

    /*
     * ==>> Above, you mapped the text pages as writable, so this code could write
     * ==>> the new text there.
     *
     * ==>> But now, you need to change the protections so that the machine can execute
     * ==>> the text.
     *
     * ==>> For each text page in region1, change the protection to (PROT_READ | PROT_EXEC).
     * ==>> If any of these page table entries is also in the TLB,
     * ==>> you will need to flush the old mapping.
     */

    for (int i = text_pg1; i < text_pg1 + li.t_npg; i++)
    {
        proc->userland_pt[i].prot = PROT_READ | PROT_EXEC;
    }
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);

    /*
     * Zero out the uninitialized data area
     */
    bzero((void *)li.id_end, li.ud_end - li.id_end);

    /*
     * Set the entry point in the process's UserContext
     */

    /*
     * ==>> (rewrite the line below to match your actual data structure)
     * ==>> proc->uc.pc = (caddr_t) li.entry;
     */
    proc->user_c.pc = (caddr_t)li.entry;

    /*
     * Now, finally, build the argument list on the new stack.
     */

    memset(cpp, 0x00, VMEM_1_LIMIT - ((int)cpp));

    *cpp++ = (char *)argcount; /* the first value at cpp is argc */
    cp2 = argbuf;
    for (i = 0; i < argcount; i++)
    { /* copy each argument and set argv */
        *cpp++ = cp;
        strcpy(cp, cp2);
        cp += strlen(cp) + 1;
        cp2 += strlen(cp2) + 1;
    }
    free(argbuf);
    *cpp++ = NULL; /* the last argv is a NULL pointer */
    *cpp++ = NULL; /* a NULL pointer for an empty envp */

    return SUCCESS;
}

void Checkpoint3TrapClock(UserContext *user_context)
{
    TracePrintf(1, "TRAPPPPP: Clock Trap.\n");
    memcpy(&current_process->user_c, user_context, sizeof(UserContext));
    runProcess();
    memcpy(user_context, &current_process->user_c, sizeof(UserContext));
    /**
     * 1. Increment clock (if we go with a global clock)
     * 2. Check delay queue, find all processes that are ready to be woken up
     *       a. This delay queue is sorted by delay()
     * 3. Move these processes from delay queue to ready queue
     * 4. return to user mode
     */
}