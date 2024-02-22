#ifndef COMMON_H
#define COMMON_H

#include <hardware.h>
#include "../lib/queue.h"

#define MAX_ARGS 50
#define MAX_ARG_LEN 256

/* Exit Codes */
#define GENERAL_EXCEPTION           -66
#define ILLEGAL_MEMORY              -131 
#define ILLEGAL_INSTRUCTION         -132 // or 4?
#define FLOATING_POINT_EXCEPTION    -136
#define MAX_KERNEL_STACK 4096

typedef struct pcb pcb_t;

typedef enum State {
    RUNNING        = 0,
    READY          = 1,
    LOCK_BLOCKED   = 2,
    CVAR_BLOCKED   = 3,
    PIPE_BLOCKED   = 4,
    DELAYED        = 5,
    DEAD           = 6,
} State_t;

typedef struct pcb {
    char* name;
    int pid;
    pcb_t *parent;
    int exit_status; // if exited, this contains exit status.
    int delayed_until; // time this pcb is delayed until
    State_t state;
    Queue_t *children;
    Queue_t *zombies;
    Queue_t * waiters; //to store pointers to pcbs of processes waiting on this one

    UserContext user_c;
    KernelContext kernel_c;

    int brk;
    int highest_text_addr;
    pte_t userland_pt[MAX_PT_LEN]; 
    pte_t kernel_stack_pt[KERNEL_STACK_MAXSIZE/PAGESIZE];
} pcb_t;

typedef struct Lock {
    int id;           // index in locks array
    int owner_pid;
    Queue_t *waiting;
} Lock_t;

typedef struct Cvar {
    int id;           // index in cvars array
    int owner_pid;
    Queue_t *waiting;
} Cvar_t;

#define PIPE_SIZE 1024

typedef struct Pipe {
    int id;                    // index in pipes array
    pcb_t* curr_reader;
    pcb_t* curr_writer;
    int read_pos;
    int write_pos;
    char buffer[PIPE_SIZE]; 
    Queue_t *readers;
} Pipe_t;

#define MAX_LOCKS 100
#define MAX_CVARS 100
#define MAX_PIPES 100

/* Queues to help indicate which resources are available (by index). */
extern Queue_t* ready_queue;
extern Queue_t* waiting_queue;
extern Queue_t* delay_queue; // This will be sorted. 
extern Queue_t* empty_locks;
extern Queue_t* empty_cvars;
extern Queue_t* empty_pipes;
extern Queue_t* empty_frames; // to track free frames

// each entry represents a terminal, and stores a linked list of strings with MAX_TERMINAL_LENGTH length each
extern Queue_t terminal_input_buffers[NUM_TERMINALS];
extern Queue_t terminal_output_buffers[NUM_TERMINALS];

extern int can_transmit_to_terminal[NUM_TERMINALS];

extern Pipe_t pipes[MAX_PIPES];
extern Lock_t locks[MAX_LOCKS];
extern Cvar_t cvars[MAX_CVARS];

extern pte_t kernel_pt[MAX_PT_LEN];

extern void* interrupt_vector_tbl[TRAP_VECTOR_SIZE];

extern int kernel_brk;

extern pcb_t *current_process;
extern pcb_t *idle_process;

// For delay and traps
extern int clock_ticks;

// to hold size of pmem
extern int pmem_size_holder;

#endif
