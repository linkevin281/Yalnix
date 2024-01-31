/**
 * Kevin Lin and Carter Sullivan
 * Yalnix
 * CS58 - Operating Systems 24W
 *
 * kernel.h
 *
 */

/* Exit Codes */
#define KILL -9
#define GENERAL_EXCEPTION -66
#define ILLEGAL_MEMORY -131
#define ILLEGAL_INSTRUCTION -132 // or 4?
#define FLOATING_POINT_EXCEPTION -136

#include <hardware.h>

#define MAX_USER_PAGETABLE 1024
#define MAX_KERNEL_STACK 4096

typedef struct Node
{
    void *data;
    Node_t *next;
    Node_t *prev;
} Node_t;

typedef struct Queue
{
    Node_t *head;
    Node_t *tail;
} Queue_t;

typedef enum State
{
    RUNNING = 0,
    READY = 1,
    LOCK_BLOCKED = 2,
    CVAR_BLOCKED = 3,
    PIPE_BLOCKED = 4,
    DELAYED = 5,
    ZOMBIE = 6,
} State_t;

typedef struct pcb
{
    int pid;
    int p_pid;
    int exit_status;
    int ticks_delayed;
    State_t state;
    Queue_t *children;
    Queue_t *zombies;

    UserContext *user_c;
    KernelContext *kernel_c;

    int brk;
    void *kernel_stack_top;
    pte_t *userland_pt[MAX_USER_PAGETABLE];
    pte_t *kernel_pt[MAX_KERNEL_STACK / PAGESIZE];
} pcb_t;

typedef struct Lock
{
    int id; // index in locks array
    int owner_pid;
    Queue *waiting;
} Lock_t;

typedef struct Cvar
{
    int id; // index in cvars array
    int owner_pid;
    Queue *waiting;
} Cvar_t;

#define PIPE_SIZE 1024

typedef struct Pipe
{
    int id; // index in pipes array
    pcb_t *curr_reader;
    pcb_t *curr_writer;
    int read_pos;
    int write_pos;
    char buffer[PIPE_SIZE];
    Queue *readers;
} Pipe_t;

#define MAX_LOCKS 100
#define MAX_CVARS 100
#define MAX_PIPES 100

int virtual_mem_enabled = 0;
int first_kernel_text_page = 0;
int first_kernel_data_page = 0;
int orig_kernel_brk_page = 0;

/* Queues to help indicate which indicies of their are empty. */
Queue_t ready_queue;
Queue_t waiting_queue;
Queue_t delay_queue; // This will be sorted.
Queue_t empty_locks;
Queue_t empty_cvars;
Queue_t empty_pipes;
Queue_t empty_frames;

// each entry represents a terminal, and stores a linked list of strings with MAX_TERMINAL_LENGTH length
Queue terminal_input_buffers[NUM_TERMINALS];
Queue terminal_output_buffers[NUM_TERMINALS];

bool can_transmit_to_terminal[NUM_TERMINALS];

Pipe_t pipes[MAX_PIPES];
Lock_t locks[MAX_LOCKS];
Cvar_t cvars[MAX_CVARS];

void (*interrupt_vector_tbl[TRAP_VECTOR_SIZE])(UserContext *user_context);

int frame_count = 0;

int brk;

pcb *current_process;

// For delay and traps
int clock_ticks = 0;

KernelContext *KCSwitch(KernelContext *kc_in, void *curr_pcb_p, void *next_pcb_p); // See 4.2
KernelContext *KCCopy(KernelContext *kc_in, void *curr_pcb_p, void *not_used);     // See 4.3

void KernelStart(char *cmd_args[])
{
    /**
     * 1. Initialize PT0
     * 2. Initialize all trap vectors()
     * 3. Write page table base and limit to registers
     * 4. Enable vMEM
     * 5. Initialize all queues()
     * 6. Load program defined in ARGS into memory
     *
     */
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

int allocateFrame()
{
    /**
     * If there are frames in empty_frames:
     *      pop the first frame off of empty_frames
     *      Return the number of this frame
     * Else:
     *      Return -1 (this could potentially be modified by evicting an existing frame - something to pursue once base functionality is working)
     */
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