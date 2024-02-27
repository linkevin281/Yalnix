#include <yuser.h>

int main() {
    int pid = Fork();

    while (1)
    {
        TracePrintf(1, "Forked process with pid: %d\n", pid);
        Pause();
    }

    Exit(0);
}