#ifndef COMMON_H
#define COMMON_H

#include <hardware.h>
#include <yalnix.h>
#include "../lib/queue.h"

#define MAX_ARGS 50
#define MAX_ARG_LEN 256
#define REDZONE_SIZE 1

/* Exit Codes */
#define GENERAL_EXCEPTION           -66
#define EXECPTION_OUT_OF_MEM        -67
#define ILLEGAL_MEM_ADDR            -131 
#define ILLEGAL_MEM_STACK_GROWTH    -134
#define ILLEGAL_INSTRUCTION         -135 // or 4?
#define FLOATING_POINT_EXCEPTION    -136


typedef struct pcb pcb_t;

typedef struct pcb {
    char* name;
    int pid;
    pcb_t *parent;
    int exit_status; // if exited, this contains exit status.
    int delayed_until; // time this pcb is delayed until
    Queue_t *children;
    Queue_t *zombies;
    Queue_t * waiters; //to store pointers to pcbs of processes waiting on this one

    UserContext user_c;
    KernelContext kernel_c;

    int brk;
    int highest_text_addr;
    pte_t userland_pt[MAX_PT_LEN]; 
    pte_t kernel_stack_pt[KERNEL_STACK_MAXSIZE/PAGESIZE];
    int is_waiting; //0 if not waiting, 1 if waiting
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

typedef struct Pipe {
    int id;                    // index in pipes array
    pcb_t* curr_reader;
    pcb_t* curr_writer;
    int read_pos;
    int write_pos;
    char buffer[PIPE_BUFFER_LEN]; 
    Queue_t *readers;
    int exists;
    int num_bytes_in_pipe;
} Pipe_t;

#define MAX_LOCKS 100
#define MAX_CVARS 100
#define MAX_PIPES 100

/* Queues to help indicate which resources are available (by index). */
extern Queue_t* ready_queue;
extern Queue_t* waiting_queue;
extern Queue_t* delay_queue; // This will be sorted. 
extern Queue_t* terminal_waiting_queue; // to store pcbs that are waiting for some I/O operation to complete
extern Queue_t* empty_locks;
extern Queue_t* empty_cvars;
extern Queue_t* empty_pipes;
extern Queue_t* empty_frames; // to track free frames

// each entry represents a terminal, and stores a linked list of strings with MAX_TERMINAL_LENGTH length each
extern Queue_t* terminal_input_buffers[NUM_TERMINALS];
extern Queue_t* terminal_output_buffers[NUM_TERMINALS];

// quesues of pcbs attempting to read and write from various terminals
extern Queue_t* want_to_read_from[NUM_TERMINALS];
extern Queue_t* want_to_write_to[NUM_TERMINALS];

extern int can_write_to_terminal[NUM_TERMINALS];
extern int can_read_from_terminal[NUM_TERMINALS];

// queues of processes waiting to read or write from pipes
extern Queue_t* pipe_write_queues[MAX_PIPES];
extern Queue_t* pipe_read_queues[MAX_PIPES];

// 0 if we can't read or write to pipe at the moment, 1 otherwise
extern int can_interact_with_pipe[MAX_PIPES];

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
