#include <yuser.h>

int main() {
    
    int pid = Fork();

    int* status_holder = (int*) malloc(sizeof(int));

    TracePrintf(1, "Forked process with pid: %d\n", pid);

    // in child, pause
    if(pid == 0){
        for(int i = 0; i < 10; i++){
            TracePrintf(1, "I am the child, and I am pausing for my parent...\n");
            Pause();
        }
        Exit(0);
    }

    else{
        for(int i = 0; i < 4; i++){
            Pause();
        }
        TracePrintf(1, "I am the parent, and will now wait for the child...\n");
        int child_pid = Wait(status_holder);
        TracePrintf(1, "DONE waiting let's go!\n");
        TracePrintf(1, "Child with pid %d and exit status %d is done.\n", child_pid, *status_holder);

    }
    
    Exit(0);
}