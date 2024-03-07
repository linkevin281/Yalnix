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

// Test case 1: Basic Lock Acquire and Release
void test_basic_lock()
{
    int lock_id = lock_init();

    int pid = Fork();
    if (pid == 0)
    {
        lock_acquire(lock_id);
        TracePrintf(1, "Looks like I (child | pid: %d) acquired the lock first. Haha.\n", GetPid());
        lock_release(lock_id);
        Exit(SUCCESS);
    }
    else if (pid > 0)
    {
        lock_acquire(lock_id);
        TracePrintf(1, "Looks like I (parent | pid: %d) acquired the lock first. Haha.\n", GetPid());
        lock_release(lock_id);
        int status;
        Wait(&status);
        if (status != SUCCESS)
        {
            Exit(FAILURE);
        }
        Exit(SUCCESS);
    }
    TracePrintf(1, "why are we here? died\n");
}

void test_double_acquire()
{
    int lock_id = lock_init();
    TracePrintf(1, "Lock id: %d\n", lock_id);
    int r1 = Acquire(lock_id);
    int r2 = Acquire(lock_id);
    int r3 = Release(lock_id);
    int r4 = Release(lock_id);
    if (r1 == SUCCESS && r2 == SUCCESS && r3 == SUCCESS && r4 == FAILURE)
    {
        Exit(SUCCESS);
    }
    else
    {
        TracePrintf(1, "R1: %d Expected: %d\n", r1, SUCCESS);
        TracePrintf(1, "R2: %d Expected: %d\n", r2, SUCCESS);
        TracePrintf(1, "R3: %d Expected: %d\n", r3, SUCCESS);
        TracePrintf(1, "R4: %d Expected: %d\n", r4, FAILURE);
        Exit(FAILURE);
    }
}

void test_mutual_exclusion()
{
    int lock_id = lock_init();
    int *arr = malloc(sizeof(int) * 10);
    TracePrintf(1, "STARTBRK: %p\n", sbrk(0));
    for (int i = 0; i < 10; i++)
    {
        TracePrintf(1, "START: Mem location of arr[%d]: %p\n", i, &arr[i]);
    }

    int main_pid = Fork();
    if (main_pid == 0)
    {
        int pid = Fork();
        if (pid == 0)
        {
            lock_acquire(lock_id);
            TracePrintf(1, "Looks like I (child | pid: %d) acquired the lock first. Haha.\n", GetPid());
            TracePrintf(1, "I'm going to write half the array\n");
            for (int i = 0; i < 5; i++)
            {
                arr[i] = 0;
            }
            TracePrintf(1, "Taking a quick break! Can't wait for Thayer break room coffee!\n");
            Pause();
            TracePrintf(1, "I'm back (child)! I'm going to write the other half of the array. Hopefully no one got to it before I did\n");
            for (int i = 5; i < 10; i++)
            {
                arr[i] = 0;
            }
            lock_release(lock_id);
            for (int i = 0; i < 10; i++)
            {
                TracePrintf(1, "END CHILD: Mem location of arr[%d]. Value: %d: %p\n", i, arr[i], &arr[i]);
            }
            Exit(SUCCESS);
        }
        else if (pid > 0)
        {

            lock_acquire(lock_id);
            TracePrintf(1, "Looks like I (parent | pid: %d) acquired the lock first. Haha.\n", GetPid());
            TracePrintf(1, "I'm going to write half the array\n");
            for (int i = 0; i < 5; i++)
            {
                arr[i] = 1;
            }
            TracePrintf(1, "Taking a quick break! Can't wait for Thayer break room coffee!\n");
            Pause();
            TracePrintf(1, "I'm back (parent)! I'm going to write the other half of the array. Hopefully no one got to it before I did\n");
            for (int i = 5; i < 10; i++)
            {
                arr[i] = 1;
            }
            lock_release(lock_id);
            for (int i = 0; i < 10; i++)
            {
                TracePrintf(1, "END PARENT: Mem location of arr[%d]. Value: %d: %p\n", i, arr[i], &arr[i]);
            }
            int status;
            Wait(&status);
            if (status == SUCCESS)
            {
                Exit(SUCCESS);
            }
            else
            {
                Exit(FAILURE);
            }
        }
    }
    if (main_pid > 0)
    {
        int status;
        Wait(&status);
        if (status == SUCCESS)
        {
            int first_element = arr[0];
            TracePrintf(1, "Mem location of arr: %p\n", arr);
            for (int i = 0; i < 10; i++)
            {
                TracePrintf(1, "MEM: Mem location of arr[%d]. Value: %d: %p\n", i, arr[i], &arr[i]);
                if (arr[i] != first_element)
                {
                    TracePrintf(1, "Array is not consistent, someone wrote to it while the lock was LOCKED\n");
                    Exit(FAILURE);
                }
                TracePrintf(1, "END MAIN: Mem location of arr[%d]. Value: %d: %p\n", i, arr[i], &arr[i]);
            }
            if (first_element == 0)
            {
                TracePrintf(1, "Looks like the child wrote to the array second. Good job!\n");
            }
            Exit(SUCCESS);
        }
        else
        {
            Exit(FAILURE);
        }
    }
}

void test_fork_inheritance()
{
    int lock_id = lock_init();
    lock_acquire(lock_id);
    int pid = Fork();
    if (pid == -1)
    {
        TracePrintf(1, "Happy happy happy. Fork failed\n");
        Exit(SUCCESS);
    }
    Exit(FAILURE);
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
        TracePrintf(1, "Waiting for %s to finish...\n", test_name);
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
    TracePrintf(1, "Running locks.c tests...\n");
    run_test(test_basic_lock, "Test 1: Basic Lock Acquire and Release");
    // What if we lock the same lock twice or unlock it twice?
    run_test(test_double_acquire, "Test 2: Double Lock Acquire and Release");
    // What if we have two processes trying to write to the same array? AND we pause halfway through?
    run_test(test_mutual_exclusion, "Test 3: Mutual Exclusion");
    // What if we fork a process WITH a lock in hand?
    run_test(test_fork_inheritance, "Test 4: Fork Inheritance");
    TracePrintf(1, "ENDING BRK: %p\n", sbrk(0));
    // What if a process dies with a lock in hand? Or multiple??
    // run_test(test_lock_leaks, "Test 5: Lock Leaks");
}
