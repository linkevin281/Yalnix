#include <hardware.h>
#include <yuser.h>

void main(void)
{
        TracePrintf(1, "Delay 3 ticks\n");
        Delay(3);
        TracePrintf(1, "Delay is OVER!\n");
        Exit(0);
}
