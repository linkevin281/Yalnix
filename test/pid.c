#include <hardware.h>
#include <yuser.h>

void main(void)
{
    while (1)
    {
        TracePrintf(1, "DoSyscallPID, PID: %d\n", GetPid());
        Pause();
    }
}
