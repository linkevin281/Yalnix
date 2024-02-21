#include <hardware.h>
#include <yuser.h>

void main(void)
{
    while (1)
    {
        TracePrintf(1, "Delay 3 ticks\n");
        Delay(3);
        Pause();
    }
}
