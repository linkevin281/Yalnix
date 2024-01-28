/**
 * Kevin Lin and Carter Sullivan
 * Yalnix
 * CS58 - Operating Systems 24W
 *
 * kernel.h
 * 
 */


/* Exit Codes */
#define KILL                        -9
#define GENERAL_EXCEPTION           -66
#define ILLEGAL_MEMORY              -131 
#define ILLEGAL_INSTRUCTION         -132 // or 4?
#define FLOATING_POINT_EXCEPTION    -136

#include <hardware.h>

#define MAX_USER_PAGETABLE 1024
#define MAX_KERNEL_STACK 4096

typedef struct Node {
    void *data;
    Node_t *next;
    Node_t *prev;
} Node_t;

typedef struct Queue {
    Node_t *head;
    Node_t *tail;
} Queue_t;

typedef enum State {
    RUNNING        = 0,
    READY          = 1,
    LOCK_BLOCKED   = 2,
    CVAR_BLOCKED   = 3,
    PIPE_BLOCKED   = 4,
    DELAYED        = 5,
    ZOMBIE         = 6,
} State_t;

typedef struct pcb {
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
    pte_t *kernel_pt[MAX_KERNEL_STACK/PAGESIZE];
} pcb_t;

typedef struct Lock {
    int id;           // index in locks array
    int owner_pid;
    Queue *waiting;
} Lock_t;

typedef struct Cvar {
    int id;           // index in cvars array
    int owner_pid;
    Queue *waiting;
} Cvar_t;

#define PIPE_SIZE 1024

typedef struct Pipe {
    int id;                    // index in pipes array
    pcb_t* curr_reader;
    pcb_t* curr_writer;
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

// arrays of strings for storing input and output for various terminals
char* terminal_input_buffers[NUM_TERMINALS];
char* terminal_output_buffers[NUM_TERMINALS];

Pipe_t pipes[MAX_PIPES];
Lock_t locks[MAX_LOCKS];
Cvar_t cvars[MAX_CVARS];

void (*interrupt_vector_tbl[TRAP_VECTOR_SIZE])(UserContext *user_context);

int frame_count = 0;

int brk;

pcb *current_process;

// For delay and traps
int clock_ticks = 0;
