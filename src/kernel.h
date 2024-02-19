/**
 * Kevin Lin and Carter Sullivan
 * Yalnix
 * CS58 - Operating Systems 24W
 *
 * kernel.h
 * 
 */
#ifndef _KERNEL_H
#define _KERNEL_H

#define ERROR                       -1

/* Exit Codes */
#define KILL                        -9
#define GENERAL_EXCEPTION           -66
#define ILLEGAL_MEMORY              -131 
#define ILLEGAL_INSTRUCTION         -132 // or 4?
#define FLOATING_POINT_EXCEPTION    -136
#define MAX_KERNEL_STACK 4096

#include <hardware.h>
#include "../lib/queue.h"

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
    int pid;
    pcb_t *parent;
    int exit_status; // if exited, this contains exit status.
    int ticks_delayed; //number of ticks until we can move this pcb out of delay queue
    State_t state;
    Queue_t *children;
    Queue_t *zombies;
    Queue_t * waiters; //to store pointers to pcbs of processes waiting on this one

    UserContext user_c;
    KernelContext kernel_c;

    int brk;
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

int frame_count = 0;

int kernel_brk;

pcb_t *current_process;
pcb_t *idle_process;

// For delay and traps
int clock_ticks = 0;

void KernelStart(char * cmd_args[], unsigned int pmem_size,
                 UserContext *uctxt);
KernelContext *KCSwitch(KernelContext *kc_in, void *curr_pcb_p, void *next_pcb_p); // See 4.2
KernelContext *KCCopy(KernelContext *kc_in, void *curr_pcb_p, void *not_used); // See 4.3
int allocateFrame();
int deallocateFrame(int frame_index);
int SetKernelBrk(void * addr);
int runProcess();
pcb_t *initIdleProcess(UserContext *uctxt, char *args[], char *name);
pcb_t *initInitProcess(UserContext *uctxt, char *args[], char *name);
pcb_t *createPCB();
int LoadProgram(char *name, char *args[], pcb_t *pcb);
void Checkpoint3TrapClock(UserContext *user_context);

#endif