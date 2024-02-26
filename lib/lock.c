/**
 * Kevin Lin and Carter Sullivan
 * Yalnix
 * CS58 - Operating Systems 24W
 *
 */


#include "lock.h"

/**
 * Creates a lock with the given id. Defined as index in arr.
*/
Lock_t *createLock(int lock_id) {
    Lock_t *lock = (Lock_t *)malloc(sizeof(Lock_t));
    if (lock == NULL) {
        return NULL;
    }
    lock->lock_id = lock_id;
    lock->owner_pcb = NULL;
    lock->is_locked = 0;
    lock->waiting_queue = createQueue();
    return lock;
}

/**
 * Acquires the lock. If lock is free, lock just gets acquired, fill owner. 
 * Otherwise, add to waiting queue, update state of process to blocked. Eventually, 
*/
int acquire(Lock_t *lock, pcb_t *pcb)
{
    if (lock->is_locked == 0) {
        lock->is_locked = 1;
        lock->owner_pcb = pcb;
        return ACQUIRE_SUCCESS;
    } else {
        pcb->state = BLOCKED;
        enqueue(lock->waiting_queue, pcb);
        return ACQUIRE_FAILED;
    }
}

/**
 * Releases the lock. If waiting queue, remove one process and move to ready queue, update state.
*/
int release(Lock_t *lock, pcb_t *pcb)
{
    if (lock->owner_pcb->pid != pcb->pid) {
        return RELEASE_FAILED;
    }
    if (lock->waiting_queue->size == 0) {
        lock->is_locked = 0;
        lock->owner_pcb = NULL;
        return RELEASE_SUCCESS;
    } else {
        Node_t *node = dequeue(lock->waiting_queue);
        pcb_t *pcb = (pcb_t *)node->data;
        free(node);
        pcb->state = READY;
        ready_queue = enqueue(ready_queue, pcb);
        return RELEASE_SUCCESS;
    }
}
