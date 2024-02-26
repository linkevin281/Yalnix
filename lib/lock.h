/**
 * Kevin Lin and Carter Sullivan
 * Yalnix
 * CS58 - Operating Systems 24W
 *
 */

#ifndef QUEUE_H
#include "queue.h"
#include "globals.h"

#define ACQUIRE_SUCCESS      0
#define ACQUIRE_FAILED      -1
#define RELEASE_SUCCESS      0
#define RELEASE_FAILED      -1

// If lock is free, lock just gets acquired, fill owner_pid
// if lock is not free, add to waiting queue, update state of process to blocked 
// if release is run, check if waiting queue is empty, move process to ready queue and update state
// If 

typedef struct Lock Lock_t;

typedef struct Lock
{
    int lock_id;
    pcb_t *owner_pcb;
    int is_locked;
    Queue_t *waiting_queue;
} Lock_t;

/**
 * Creates a lock with the given id. Defined as index in arr.
*/
Lock_t *createLock(int lock_id);

/**
 * Acquires the lock. If lock is free, lock just gets acquired, fill owner_pid. 
 * Otherwise, add to waiting queue, update state of process to blocked. Eventually, 
*/
int acquire(Lock_t *lock, pcb_t *pcb);

/**
 * Releases the lock. If waiting queue, remove one process and move to ready queue, update state.
*/
int release(Lock_t *lock, pcb_t *pcb);
#endif