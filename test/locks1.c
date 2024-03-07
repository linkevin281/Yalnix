#include <yuser.h>
#include <stdio.h>

#define SUCCESS 0
#define FAILURE -1

int lock_acquire(int lock_id)
{
    if (Acquire(lock_id) == -1)
    {
        TracePrintf(1, "Lock not acquired\n");
        Exit(FAILURE);
    }
    else
    {
        TracePrintf(1, "Lock acquired\n");
        return SUCCESS;
    }
}
int lock_release(int lock_id)
{
    if (Release(lock_id) == -1)
    {
        TracePrintf(1, "Lock not released\n");
        Exit(FAILURE);
    }
    else
    {
        TracePrintf(1, "Lock released\n");
        return SUCCESS;
    }
}

int lock_init()
{
    int *lock_id = malloc(sizeof(int));
    int r = LockInit(lock_id);
    if (r == -1)
    {
        TracePrintf(1, "No Locks Left\n");
        return -1;
    }
    TracePrintf(1, "Lock id: %d initalized\n", *lock_id);
    return *lock_id;
}

void test_lock_leaks()
{
    int pid = Fork();
    if (pid == 0)
    {
        TracePrintf(1, "I'm the child, greedily consuming all locks now.\n");
        int lock_id;
        
        while ((lock_id = lock_init()) != -1)
        {
            continue;
        }
        TracePrintf(1, "Looks like I've consumed all the locks (Total: %d) muhahahahaha. I'm pausing.\n", lock_id + 1);
        Pause();
        TracePrintf(1, "I'm back (child)! Dying now\n");
        Exit(SUCCESS);
    }
    else if (pid > 0)
    {
        TracePrintf(1, "I'm the parent, pausing to let the child consume all the locks\n");
        Pause();
        TracePrintf(1, "I'm the parent, trying to consume a lock now!");
        int lock_id = lock_init();
        if (lock_id != -1)
        {
            TracePrintf(1, "How did I get a lock? I'm a parent! I'm going to die now.\n");
            Exit(FAILURE);
        }
        TracePrintf(1, "Looks like I didn't get a lock. Waiting for the child\n");
        int status;
        Wait(&status);
        TracePrintf(1, "I'm back (parent)! Trying again\n");
        lock_id = lock_init();
        if (lock_id == -1)
        {
            TracePrintf(1, "Why are there no locks left? I'm going to die now.\n");
            Exit(FAILURE);
        }
        Exit(SUCCESS);
    }
}

void run_test(void (*test_func)(), char *test_name)
{
    int pid = Fork();
    if (pid == 0)
    {
        TracePrintf(1, "Running %s...\n", test_name);
        test_func();
        TracePrintf(1, "ERROR: TEST FUNCTION DID NOT EXIT\n");
        Exit(FAILURE);
    }
    else if (pid > 0)
    {
        int status;
        Wait(&status);
        if (status == SUCCESS)
        {
            TracePrintf(1, "%s passed\n", test_name);
        }
        else
        {
            TracePrintf(1, "%s failed. Exit status %d\n", test_name, status);
        }
    }
}

void main(void)
{
    TracePrintf(1, "Running locks1.c tests...\n");
    // What if a process dies with a lock in hand? Or multiple??
    run_test(test_lock_leaks, "Test 5: Lock Leaks");
}
