/**
 * Kevin Lin and Carter Sullivan
 * Yalnix
 * CS58 - Operating Systems 24W
 *
 */


#ifndef QUEUE_H
#include "queue.h"
#include "lock.h"
#include <hardware.h>

#define SUCCESS            0
#define ERROR_NOT_OWNER   -2

typedef struct Cvar Cvar_t;

typedef struct Cvar
{
    int cvar_id;
    Queue_t *waiting_queue;
} Cvar_t;

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
int wait(Cvar_t *cvar, Lock_t *lock, pcb_t *caller, UserContext *uctxt);

/**
 * Signals the condition variable. Removes one process from the waiting queue and moves it to the ready queue. 
 *   Sets state to be ready
*/
int signal(Cvar_t *cvar);

/**
 * Broadcasts the condition variable. Removes all processes from the waiting queue and moves them to the ready queue. 
*/
int broadcast(Cvar_t *cvar);

#endif