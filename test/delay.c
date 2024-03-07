#include <hardware.h>
#include <yuser.h>

void main(void)
{
        TracePrintf(1, "Delay 3 ticks\n");
        Delay(3);
        TracePrintf(1, "Delay of 3 ticks is OVER!\n");
        TracePrintf(1, "Delay 5 ticks\n");
        Delay(5);
        TracePrintf(1, "Delay of 5 ticks is OVER!\n");
        int pid = Fork();
        if(pid == 0){
        TracePrintf(1, "child delaying 5 seconds...\n");
        Delay(5);
        TracePrintf(1, "child delay OVER!...\n");
        Exit(0);
        }
        TracePrintf(1, "parent delaying 7 ticks...\n");
        Delay(7);
        TracePrintf(1, "parent delay OVER!...\n");
        Exit(0);
}

