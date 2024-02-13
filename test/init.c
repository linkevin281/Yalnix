#include <hardware.h>

void main(void)
{
    while (1)
    {
        TracePrintf(1, "DoInit\n");
        Pause();
    }
}
