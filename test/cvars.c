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

int cvar_init()
{
    int *cvar_id = malloc(sizeof(int));
    int r = CvarInit(cvar_id);
    if (r == -1)
    {
        TracePrintf(1, "No Cvars Left\n");
        return -1;
    }
    TracePrintf(1, "Cvar id: %d initalized\n", *cvar_id);
    return *cvar_id;
}
int cvar_wait(int cvar_id, int lock_id)
{
    if (CvarWait(cvar_id, lock_id) == -1)
    {
        TracePrintf(1, "Cvar not waited\n");
        Exit(FAILURE);
    }
    else
    {
        TracePrintf(1, "Cvar waited\n");
        return SUCCESS;
    }
}
int cvar_signal(int cvar_id)
{
    if (CvarSignal(cvar_id) == -1)
    {
        TracePrintf(1, "Cvar not signaled\n");
        Exit(FAILURE);
    }
    else
    {
        TracePrintf(1, "Cvar signaled\n");
        return SUCCESS;
    }
}
int cvar_broadcast(int cvar_id)
{
    if (CvarBroadcast(cvar_id) == -1)
    {
        TracePrintf(1, "Cvar not broadcasted\n");
        Exit(FAILURE);
    }
    else
    {
        TracePrintf(1, "Cvar broadcasted\n");
        return SUCCESS;
    }
}
void test_basic_cvar()
{
    int cvar_id = cvar_init();
    int lock_id = lock_init();
    int pid = Fork();
    if (pid == 0)
    {
        lock_acquire(lock_id);
        TracePrintf(1, "I'm the child, waiting on the cvar. I hopefully release the lock.\n");
        cvar_wait(cvar_id, lock_id);
        TracePrintf(1, "Made it out, I'm the child\n");
        lock_release(lock_id);
        Exit(SUCCESS);
    }
    else if (pid > 0)
    {
        TracePrintf(1, "I'm the parent, pausing for the child to catch up (he has stubbylegs)\n");
        Pause();
        lock_acquire(lock_id);
        TracePrintf(1, "I'm the parent, signaling the cvar!\n");
        cvar_signal(cvar_id);
        lock_release(lock_id);
        TracePrintf(1, "I'm the parent, releasing the lock!\n");
        int status = 0;
        Wait(&status);
        Exit(status);
    }
}
void test_no_waiters()
{
    int cvar_id = cvar_init();
    int lock_id = lock_init();
    lock_acquire(lock_id);
    TracePrintf(1, "I'm the parent, signaling the cvar!\n");
    if (cvar_signal(cvar_id) == -1)
    {
        Exit(FAILURE);
    }
    lock_release(lock_id);
    TracePrintf(1, "I'm the parent, releasing the lock!\n");
    Exit(SUCCESS);
}
void test_broadcast()
{
    int cvar_id = cvar_init();
    int lock_id = lock_init();

    for (int i = 0; i < 5; i++)
    {
        int pid = Fork();
        if (pid == 0)
        {
            lock_acquire(lock_id);
            TracePrintf(1, "I'm a child PID: %d, I cant wait to be woken! Time to wait on the cvar\n", GetPid());
            cvar_wait(cvar_id, lock_id);
            TracePrintf(1, "Made it out, I'm child PID: %d\n", GetPid());
            Custom0(0, 0, 0, 0);
            lock_release(lock_id);
            Exit(SUCCESS);
        }
    }
    Custom0(0, 0, 0, 0);
    for (int i = 0; i < 5; i++)
    {
        Pause();
        TracePrintf(1, "I'm the parent, just taking my time to wait for my children to grow up.\n");
    }
    lock_acquire(lock_id);
    TracePrintf(1, "I'm the parent, broadcasting the cvar!\n");
    cvar_broadcast(cvar_id);
    lock_release(lock_id);
    int status;
    for (int i = 0; i < 5; i++)
    {
        int pid = Wait(&status);
        TracePrintf(1, "Child count: %d, PID: %d exited with status %d\n", i, pid, status);
        if (status != SUCCESS)
        {
            Exit(FAILURE);
        }
    }
    TracePrintf(1, "All children exited with status %d\n", SUCCESS);
    Custom0(0, 0, 0, 0);
    Exit(SUCCESS);
}
void run_test(void (*test_func)(), char *test_name)
{
    int pid = Fork();
    if (pid == 0)
    {
        TracePrintf(1, "Running %s...\n", test_name);
        test_func();
        TracePrintf(1, "ERROR: TEST FUNCTION DID NOT EXIT, my PID: %d\n", GetPid());
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
    // Testing some basics
    run_test(test_basic_cvar, "Test 1: Test Basic Cvar Init, Wait, Signal");
    // Testing if no waiters exist
    run_test(test_no_waiters, "Test 2: Test No Waiters but Signaling");
    // Testing broadcast
    run_test(test_broadcast, "Test 3: Test Broadcast");

    Exit(SUCCESS);
}
