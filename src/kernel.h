/**
 * Kevin Lin and Carter Sullivan
 * Yalnix
 * CS58 - Operating Systems 24W
 *
 * kernel.h
 * 
 */

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
    State_t state;
    Queue_t *children;
    Queue_t *zombies;

    UserContext *user_c;
    KernelContext *kernel_c;

    void *kernel_stack_top;
    pte_t *userland_pt[MAX_USER_PAGETABLE];
    pte_t *kernel_pt[MAX_KERNEL_STACK/PAGESIZE];
} pcb_t;

typedef struct Lock {
    int owner_pid;
    Queue *waiting;
} Lock_t;

typedef struct Cvar {
    int owner_pid;
    Queue *waiting;
} Cvar_t;

#define PIPE_SIZE 1024

typedef struct Pipe {
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
Queue_t delay_queue;
Queue_t empty_locks;
Queue_t empty_cvars;
Queue_t empty_pipes;
Queue_t empty_frames;

Pipe_t pipes[MAX_PIPES];
Lock_t locks[MAX_LOCKS];
Cvar_t cvars[MAX_CVARS];

int frame_count = 0;

int brk;



pcb *current_process;

