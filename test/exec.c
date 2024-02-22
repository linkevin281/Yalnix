#include <hardware.h>
#include <yuser.h>

void main(void)
{  
    char **argv = (char **)malloc(2 * sizeof(char *));
    argv[0] = "delay";
    argv[1] = NULL;
    TracePrintf(1, "Exec into Delay\n");
    Exec("./test/delay", argv);
    Exit(0);
}
