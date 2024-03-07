#include <yuser.h>
#include <ykernel.h>
#include "../src/kernel.h"

void main(void)
{
    int res;

    // testing with memory below region 1
    char* below_region_1 = (char*) VMEM_0_LIMIT - 10;
    char** new_arr = malloc(8 * 8);
    res = Exec(below_region_1, new_arr);
    if (res == -1){
        TracePrintf(1, "Protection passed for access to region 0\n");
    }
    free(new_arr);

    // testing with memory in region 1 text
    char* region_1_text = (char*) VMEM_1_BASE + 10;
    res = TtyWrite(0, below_region_1, 1);
    if (res == -1){
        TracePrintf(1, "Protection passed for access to region 1 text\n");
    }
}