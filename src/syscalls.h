/**
 * Kevin Lin and Carter Sullivan
 * Yalnix
 * CS58 - Operating Systems 24W
 *
 * syscalls.h
 *
 */

#include "globals.h"

/* Creates a new process that is a copy of the calling process. */
int Y_Fork(void);
/* Replaces the currently running process with a new process image. */
int Y_Exec(char *filename, char *argv[]);
/* Terminates the calling process with exit code {status} */
int Y_Exit(int status);
/* Waits for a child process to terminate, returns the child's PID and stores status at ptr. */
int Y_Wait(int *status);
/* Returns the PID of the calling process. */
int Y_Getpid(void);
/* Changes the location of the program break. */
int Y_Brk(void *addr);
/* Delays the calling process for a specified number of clock ticks. */
int Y_Delay(int num_ticks);
int Y_Ttyread(int tty_id, void *buf, int len);
int Y_Ttywrite(int tty_id, void *buf, int len);
int Y_Pipeinit(int *pipe_idp);
int Y_Piperead(int pipe_id, void *buf, int len);
int Y_Pipewrite(int pipe_id, void *buf, int len);
int Y_LockInit(int *lock_idp);
int Y_Acquire(int lock_id);
int Y_Release(int lock_id);
int Y_CvarInit(int *cvar_idp);
int Y_CvarSignal(int cvar_id);
int Y_CvarBroadcast(int cvar_id);
int Y_Cvarwait(int cvar_id, int lock_id);
int Y_Reclaim(int id);
int Y_Custom0(void);

/**
 * Creates a lock with the given id. Defined as index in arr.
 */
Lock_t *createLock(int lock_id);

/**
 * Acquires the lock. If lock is free, lock just gets acquired, fill owner_pid.
 * Otherwise, add to waiting queue, update state of process to blocked. Eventually,
 */
int acquireLock(Lock_t *lock, pcb_t *pcb);

/**
 * Releases the lock. If waiting queue, remove one process and move to ready queue, update state.
 */
int releaseLock(Lock_t *lock, pcb_t *pcb);

/**
 * Creates a condition variable with the given id. Defined as MAX_LOCKS + index in arr.
 */
Cvar_t *createCvar(int cvar_id);

/**
 * Waits on condition variable. Adds itself to waiting queue.
 *   Will release the lock, move one process off the waiting queue
 *   and then runProcess to the next process in the ready queue. When it wakes in here
 *   it will try to reacquire the lock. Once it has the lock, it will continue running.
 *   Also checks if this process owns the lock it is trying to release.
 */
int cWait(Cvar_t *cvar, Lock_t *lock, pcb_t *caller, UserContext *uctxt);

/**
 * Signals the condition variable. Removes one process from the waiting queue and moves it to the ready queue.
 *   Sets state to be ready
 */
int cSignal(Cvar_t *cvar, pcb_t *caller);

/**
 * Broadcasts the condition variable. Removes all processes from the waiting queue and moves them to the ready queue.
 */
int cBroadcast(Cvar_t *cvar, pcb_t *caller);
