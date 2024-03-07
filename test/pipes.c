#include <hardware.h>
#include <yuser.h>

void main(void)
{

    // standard write and read
    TracePrintf(1, "In pipes test!\n");
    int* pipe_holder = (int*) malloc(sizeof(int));
    int temp = PipeInit(pipe_holder);
    int buf_len = 256;
    int* holder = malloc(sizeof(int));

    int pid = Fork();

    if(pid == 0){
        TracePrintf(1, "I am the child and I will pause for a bit...\n");
        Delay(4);
        TracePrintf(1, "I am the child, and I'm now trying to read from the pipe...\n");
        int len = 100;
        void* buf = malloc(len);
        PipeRead(*pipe_holder, buf, len);
        TracePrintf(1, "I am the child, and this is what I read from the pipe: %s\n", buf);
        Exit(0);
    }
        TracePrintf(1, "I am the parent and I am about to write to the pipe...\n");
        char* parent_str = "Wassup child!!!\n";
        PipeWrite(*pipe_holder, parent_str, strlen(parent_str) + 1);
        TracePrintf(1, "I am the parent, and I just wrote to the pipe!\n");
        Wait(holder);

    pid = Fork();

    // double writes
    if(pid == 0){
        TracePrintf(1, "I am the child and I will pause for a bit...\n");
        Delay(6);
        TracePrintf(1, "I am the child, and I'm now trying to read from the pipe...\n");
        int len = 100;
        void* buf = malloc(len);
        PipeRead(*pipe_holder, buf, len);
        TracePrintf(1, "I am the child, and this is what I read from the pipe (with double writes): %s\n", buf);
        Exit(0);
    }
        char* parent_1 = "cookie ";
        char* parent_2 = "monster\n";
        TracePrintf(1, "I am the parent and I am about to do my first write to the pipe...\n");
        PipeWrite(*pipe_holder, parent_1, strlen(parent_1));
        TracePrintf(1, "I am the parent and I am about to do my second write to the pipe...\n");
        PipeWrite(*pipe_holder, parent_2, strlen(parent_2) + 1);
        TracePrintf(1, "I am the parent, and I just finished both writes to the pipe!\n");
        Wait(holder);

    // double reads
    pid = Fork();

    if(pid == 0){
        TracePrintf(1, "I am the child and I will pause for a bit...\n");
        Delay(6);
        TracePrintf(1, "I am the child, and I'm now trying to read from the pipe...\n");
        int len = 100;
        void* buf_1 = malloc(len);
        void* buf_2 = malloc(len);
        PipeRead(*pipe_holder, buf_1, 7);
        TracePrintf(1, "I am the child, and this is what I read from the pipe on my first read: %s\n", buf_1);
        PipeRead(*pipe_holder, buf_2, 9);
        TracePrintf(1, "I am the child, and this is what I read from the pipe on my second read: %s\n", buf_2);
        Exit(0);
    }
        TracePrintf(1, "I am the parent and I am about to write to the pipe...\n");
        parent_str = "Read me twice!\n";
        PipeWrite(*pipe_holder, parent_str, strlen(parent_str) + 1);
        TracePrintf(1, "I am the parent, and I just wrote to the pipe!\n");
        Wait(holder);

    // wraparound
    TracePrintf(1, "fuckerrr\n");
    pipe_holder = (int*) malloc(sizeof(int));
    temp = PipeInit(pipe_holder);


    pid = Fork();
    TracePrintf(1, "fuckerrr 2\n");
    if(pid == 0){
        TracePrintf(1, "I am the child and I will pause for a bit...\n");
        Delay(6);
        TracePrintf(1, "I am the child, and I'm now trying to read from the pipe...\n");
        char* buf = malloc(buf_len);
        int len = PipeRead(*pipe_holder, buf, 128);
        TracePrintf(1, "I am the child, and this number should be 128: %d\n", len);
        TracePrintf(1, "I am the child, and this is what I read from the pipe when testing wraparound for the first time: %s\n", buf);
        char* buf_7 = malloc(buf_len);
        len = PipeRead(*pipe_holder, buf_7, 200);
        TracePrintf(1, "I am the child, and this number should be 200: %d\n", len);
        buf[len] = '\0';
        TracePrintf(1, "I am the child, and this is what I read from the pipe when testing wraparound for the second time: %s\n", buf_7);
        Exit(0);
    }
        TracePrintf(1, "I am the parent and I am about to write 128 bytes to the pipe...\n");
        char parent_str_4[128];
        for(int i = 0; i < 127; i++){
            parent_str_4[i] = 'a';
        }
        parent_str_4[127] = '\0';
        PipeWrite(*pipe_holder, parent_str_4, 128);
        TracePrintf(1, "I am the parent, and I just wrote 128 characters to the pipe!\n");
        Delay(6);
        char parent_str_2 [200];
        for(int i = 0; i < 199; i++){
            parent_str_2[i] = 'b';
        }
        parent_str_2[199] = '\0';
        PipeWrite(*pipe_holder, parent_str_2, 200);
        TracePrintf(1, "I am the parent, and I just wrote 200 characters to the pipe!\n");
        Wait(holder);

   // multiple processes attempting to write to same pipe
    pipe_holder = (int*) malloc(sizeof(int));
    temp = PipeInit(pipe_holder);


    pid = Fork();

    if(pid == 0){
        TracePrintf(1, "I am child 1 and I will try to write to the pipe!...\n");
        char* buf = "child 1 writing!\n";
        int len = PipeWrite(*pipe_holder, buf, strlen(buf) + 1);
        TracePrintf(1, "fuck num bytes written by child 1: %d", len);
        Exit(0);
    }
        int pid_2 = Fork();
        if(pid_2 == 0){
        TracePrintf(1, "I am child 2 and I will try to write to the pipe!...\n");
        char* buf = "child 2 writing!\n";
        int len = PipeWrite(*pipe_holder, buf, strlen(buf) + 1);
        TracePrintf(1, "fuck num bytes written by child 2: %d", len);
        Exit(0);
        }
        TracePrintf(1, "I am the parent and I am about to read from the pipe wahoo...\n");
        char* parent_reader = malloc(30);
        PipeRead(*pipe_holder, parent_reader, 30);
        TracePrintf(1, "I am the parent, and after simultaneous writes from my children, I read: %s\n", parent_reader);



    // multiple processes attempting to read from same pipe
    pipe_holder = (int*) malloc(sizeof(int));
    temp = PipeInit(pipe_holder);


    pid = Fork();

    if(pid == 0){
        Delay(4);
        char buf [5];
        TracePrintf(1, "Child 1 gonna try to read from pipe..\n");
        int len = PipeRead(*pipe_holder, buf, 5);
        TracePrintf(1, "In simultaneous read, got: %s", buf);
        Exit(0);
    }
        pid_2 = Fork();
        if(pid_2 == 0){
        Delay(4);
        char buf [5];
        TracePrintf(1, "Child 2 gonna try to read from pipe..\n");
        int len = PipeRead(*pipe_holder, buf, 5);
        TracePrintf(1, "In simultaneous read, got: %s", buf);
        Exit(0);
        }
        TracePrintf(1, "I am the parent and I am about to read from the pipe wahoo...\n");
        char* parent_str_yay = "doge";
        PipeWrite(*pipe_holder, parent_str_yay, strlen(parent_str_yay) + 1);
        TracePrintf(1, "I am the parent, and just wrote to the pipe so my children can try reading...\n");
        Delay(12);
        TracePrintf(1, "I am the parent, and I will now write to the pipe so my second waiting child can be done waiting...\n");
        char* parent_str_yiy = "boge";
        int len = PipeWrite(*pipe_holder, parent_str_yiy, 5);



        // testing reuse of pipes
        char* to_add = "a";
        for(int i = 0; i < 120; i++){
            int* pipe_holder_1 = (int*) malloc(sizeof(int));
            int temp = PipeInit(pipe_holder_1);
            char* str = malloc(2);
            TracePrintf(1, "In iteration %d\n", i);
            memcpy(str, to_add, 2);
            int pid = Fork();
            if(pid == 0){
                PipeWrite(*pipe_holder_1, str, strlen(str) + 1);
                free(str);
                Exit(0);
            }
            char holder[2];
            PipeRead(*pipe_holder_1, holder, 2);
            TracePrintf(1, "On iteration %d, just did my read\n", i);
        }
}
