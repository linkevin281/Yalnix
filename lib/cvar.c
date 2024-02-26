/**
 * Kevin Lin and Carter Sullivan
 * Yalnix
 * CS58 - Operating Systems 24W
 *
 */

#include "cvar.h"
#include "kernel.h"

/**
 * Creates a condition variable with the given id. Defined as MAX_LOCKS + index in arr.
 */
Cvar_t *createCvar(int cvar_id)
{
    Cvar_t *cvar = (Cvar_t *)malloc(sizeof(Cvar_t));
    if (cvar == NULL)
    {
        return NULL;
    }
    cvar->cvar_id = cvar_id;
    cvar->waiting_queue = createQueue();
    return cvar;
}

/**
 * Waits on condition variable. Adds itself to waiting queue.
 *   Will release the lock, move one process off the waiting queue
 *   and then runProcess to the next process in the ready queue. When it wakes in here
 *   it will try to reacquire the lock. Once it has the lock, it will continue running.
 *   Also checks if this process owns the lock it is trying to release.
 */
int wait(Cvar_t *cvar, Lock_t *lock, pcb_t *caller, UserContext *user_context)
{
    if (lock->owner_pcb->pid != caller->pid)
    {
        return ERROR_NOT_OWNER;
    }
    release(lock, lock->owner_pcb);
    enqueue(cvar->waiting_queue, lock->owner_pcb);

    memcpy(&current_process->user_c, user_context, sizeof(UserContext));
    runProcess();
    memcpy(user_context, &current_process->user_c, sizeof(UserContext));

    acquire(lock, lock->owner_pcb);
    return SUCCESS;
}
/**
 * Signals the condition variable. Removes one process from the waiting queue and moves it to the ready queue.
 *   Sets state to be ready
 */
int signal(Cvar_t *cvar, pcb_t *caller)
{
    if (cvar->waiting_queue->size == 0)
    {
        return SUCCESS;
    }
    Node_t *node = dequeue(cvar->waiting_queue);
    pcb_t *pcb = (pcb_t *)node->data;
    free(node);
    pcb->state = READY;
    enqueue(ready_queue, pcb);
    return SUCCESS;
}

/**
 * Broadcasts the condition variable. Removes all processes from the waiting queue and moves them to the ready queue.
 */
int broadcast(Cvar_t *cvar) 
{
    while (cvar->waiting_queue->size > 0)
    {
        Node_t *node = dequeue(cvar->waiting_queue);
        pcb_t *pcb = (pcb_t *)node->data;
        free(node);
        pcb->state = READY;
        enqueue(ready_queue, pcb);
    }
    return SUCCESS;
}