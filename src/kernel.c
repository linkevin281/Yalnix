/**
 * Kevin Lin and Carter Sullivan
 * Yalnix
 * CS58 - Operating Systems 24W
 *
 * kernel.h
 *
 */

#include "kernel.h"
#include <yalnix.h>
#include <hardware.h>

KernelContext *KCSwitch(KernelContext *kc_in, void *curr_pcb_p, void *next_pcb_p); // See 4.2
KernelContext *KCCopy(KernelContext *kc_in, void *curr_pcb_p, void *not_used);     // See 4.3

void KernelStart(char *cmd_args[], unsigned int pmem_size,
                 UserContext *uctxt)
{
    TracePrintf(1, "KernelStart called: initial values \n pmem_size: %d\n _first_kernel_text_page: %d\n _first_kernel_data_page: %d\n _orig_kernel_brk_page: %d\n", pmem_size, _first_kernel_text_page, _first_kernel_data_page, _orig_kernel_brk_page);
    kernel_brk = _orig_kernel_brk_page << PAGESHIFT;
    TracePrintf(1, "KernelStart: kernel_brk: %p\n", kernel_brk);

    // 1. Create Frame Queue
    // incrementing backwards, so that lowest-addresed frames are at front of the queue
    empty_frames = createQueue();
    for (int i = 0; i < (int)(pmem_size / PAGESIZE); i++)
    {
        if (enqueueBack(empty_frames, &i, sizeof(int)) == -1)
        {
            TracePrintf(1, "Error: Could not enqueue frame %d into empty_frames\n", i);
        }
    }
    TracePrintf(1, "Empty frames queue created\n");

    for (int j = _first_kernel_text_page; j < _first_kernel_data_page; j++)
    {
        TracePrintf(1, "Allocating frame for kernel_pt\n");
        // allocate frame -> (frame number/index) -> add to kernel_pt
        unsigned long pt_index = removeFrameNode(empty_frames, j);
        kernel_pt[pt_index].pfn = pt_index;
        kernel_pt[pt_index].prot = PROT_READ | PROT_EXEC;
        kernel_pt[pt_index].valid = 1;
    }

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

    // for (int i = 0; i < (int)pmem_size / PAGESIZE; i++)
    // {
    //     int frame = empty_frames;
    // }
}

KernelContext *KCSwitch(KernelContext *kc_in, void *curr_pcb_p, void *next_pcb_p)
{
    /**
     * - This function is called by KernelContextSwitch() in the hardware.
     * - Our kernel uses KernelContextSwitch() to switch between processes upon
     *      when the current process is done executing or some trap/wait other
     *      event occurs.
     *
     * KC_in is a copy of the current process's kernel context.
     * 1. Copy KC into the current process's pcb
     * 2. Copy kernel PT into the current process's pcb
     * 3. Set the kernel state to be KC of next process
     * 4. Set the kernel PT to be the next process's PT
     * 5. Flush the TLB
     * 6. Return the next process's kernel context
     *
     */
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

int deallocateFrame(int frame_index)
{
    /**
     * Add a node with frame_index to empty_frames
     * NOTE: user is responsible for flushing TLB after calling!
     */
}

int runNewProcess()
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
    // Check if addr exceeds kernel stack pointer, if so, ERROR
    if (ReadRegister(REG_VM_ENABLE) == 1)
    {
        if ((unsigned int)addr > (unsigned int)current_process->kernel_stack_bottom)
        {
            return ERROR;
        }
    }
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
            int frame_index = removeFrameNode(empty_frames, i); // One liner for virtual and non-virtual memory
            if (frame_index == -1)
            {
                return ERROR;
            }
            kernel_pt[frame_index].pfn = frame_index;
            kernel_pt[frame_index].valid = 1;
            TracePrintf(1, "Allocated frame %d for kernel_pt at memory address %p\n", frame_index, frame_index << PAGESHIFT);
        }
        kernel_brk = UP_TO_PAGE(addr);
        TracePrintf(1, "SetKernelBrk: kernel_brk: %p\n", kernel_brk);

    }
    return 0;
}
