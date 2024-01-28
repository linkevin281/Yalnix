#include <hardware.h>

#define MAX_USER_PAGETABLE 1024
#define MAX_KERNEL_STACK 4096


typedef struct _Node {
    void *data;
    Node *next;
    Node *prev;
} Node;

typedef struct _Queue {
    Node *head;
    Node *tail;
} Queue;

typedef struct _pcb {
    int pid;
    int p_pid;
    Queue *children;
    Queue *zombies;

    UserContext *user_c;
    KernelContext *kernel_c;

    void *kernel_stack_top;
    pte *userland_pt[MAX_USER_PAGETABLE];
    pte *kernel_pt[MAX_KERNEL_STACK/PAGESIZE];
} pcb;

typedef struct _Lock {
    int owner_pid;
    Queue *waiting;
} Lock;

typedef struct _Cvar {
    int owner_pid;
    Queue *waiting;
} Cvar;

#define PIPE_SIZE 1024

typedef struct Pipe {
    int read_pos;
    int write_pos;
    char buffer[PIPE_SIZE];
    Queue *readers;
} Pipe;

#define MAX_LOCKS 100
#define MAX_CVARS 100
#define MAX_PIPES 100

int virtual_mem_enabled = 0;
int first_kernel_text_page = 0;
int first_kernel_data_page = 0;
int orig_kernel_brk_page = 0;

Queue ready_queue;
Queue delay_queue;
Queue empty_locks;
Queue empty_cvars;
Queue empty_pipes;
Queue empty_frames;
int frame_count = 0;

Pipe pipes[MAX_PIPES];
Lock locks[MAX_LOCKS];
Cvar cvars[MAX_CVARS];

int brk;



pcb *current_process;

