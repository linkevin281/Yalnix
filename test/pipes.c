#include <hardware.h>
#include <yuser.h>

void main(void)
{
    TracePrintf(1, "In pipes test!\n");
    int* pipe_holder = (int*) malloc(sizeof(int));
    int temp = PipeInit(pipe_holder);

    int pid = Fork();

    if(pid == 0){
        TracePrintf(1, "I am the child and I will pause for a bit...\n");
        Delay(4);
        TracePrintf(1, "I am the child, and I'm now trying to read from the pipe...\n");
        int len = 100;
        void* buf = malloc(len);
        PipeRead(*pipe_holder, buf, len);
        TracePrintf(1, "I am the child, and this is what I read from the pipe: %s\n", buf);
    }
    else{
        TracePrintf(1, "I am the parent and I am about to write to the pipe...\n");
        char* parent_str = "Wassup child!!!\n";
        PipeWrite(*pipe_holder, parent_str, strlen(parent_str) + 1);
        TracePrintf(1, "I am the parent, and I just wrote to the pipe!\n");
    }
    
}
