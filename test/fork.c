#include <yuser.h>

int main() {
    int pid = Fork();

    TracePrintf(1, "Forked process with pid: %d\n", pid);
    Exit(0);
}