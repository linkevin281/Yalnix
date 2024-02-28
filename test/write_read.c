#include <hardware.h>
#include <yuser.h>

void main(void)
{

     TtyWrite(0, "hi\n", 4);



    // // Writing a line of more than max length
    // char* longer_than_max = malloc(TERMINAL_MAX_LINE + 10);
    // for(int i = 0; i < (TERMINAL_MAX_LINE + 10); i++){
    //     if (i < TERMINAL_MAX_LINE + 9){
    //         longer_than_max[i] = 'a';
    //     }
    //     else{
    //         longer_than_max[i] = '\n';
    //     }
    // }
    // TtyWrite(0, longer_than_max, TERMINAL_MAX_LINE + 10);
    // TracePrintf(1, "LET'S GOOOOO WROTE LONGER THAN MAX!!!\n");




    // take some input and print result
    char* prompt = "Give me input:";
    TtyWrite(0, prompt, strlen(prompt) + 1);
    char* buf = malloc(TERMINAL_MAX_LINE - 1);
    TtyRead(0, buf, TERMINAL_MAX_LINE - 1);
    TracePrintf(1, "TGT past reading!\n");
    TracePrintf(1, "given that, buf is now: %s", buf);
    char* give_to_user = "I just read your input as:\n";
    int res;
    res = TtyWrite(0, give_to_user, strlen(give_to_user) + 1);
    TracePrintf(1, "in WR test, res of writing to terminal: %d\n", res);
    res = TtyWrite(0, buf, strlen(buf) + 1);
    TracePrintf(1, "in WR test, res of writing to terminal second time: %d\n", res);


    // take some input from one terminal, print it to another
    char* prompt_2 = "Give me input, and I will print it to terminal 1: ";
    char* buf_2 = malloc(TERMINAL_MAX_LINE);
    TtyWrite(0, prompt_2, strlen(prompt_2) + 1);
    int len = TtyRead(0, buf_2, TERMINAL_MAX_LINE);
    res = TtyWrite(1, buf_2, len);
    TracePrintf(1, "in WR test, res of writing to terminal 1 with input from terminal 0: %d\n", res);

    // take more input from terminal 0, then print it back out
    TtyWrite(0, prompt, strlen(prompt) + 1);
    char* buf_3 = malloc(TERMINAL_MAX_LINE);
    len = TtyRead(0, buf_3, TERMINAL_MAX_LINE);
    res = TtyWrite(0, give_to_user, strlen(give_to_user) + 1);
    res = TtyWrite(0, buf_3, len);
    TracePrintf(1, "in WR test, res of writing to terminal second time: %d\n", res);

    // test delayed I/O
    char* new_prompt = "You have about 10 seconds to type some random stuff and hit enter. Do it!\n";
    TtyWrite(0, new_prompt, strlen(new_prompt) + 1);
    char* buf_4 = malloc(TERMINAL_MAX_LINE);
    Delay(25);
    len = TtyRead(0, buf_4, TERMINAL_MAX_LINE);
    char* tell_user = "I read in: ";
    res = TtyWrite(0, tell_user, strlen(tell_user) + 1);
    res = TtyWrite(0, buf_4, len);



    // multiple programs trying to read from same terminal
    char* fork_prompt = "forking. Enter some input here: ";
    TtyWrite(0, fork_prompt, strlen(fork_prompt) + 1);
    int pid = Fork();
    char* buf_5 = malloc(TERMINAL_MAX_LINE);

    if(pid == 0){
        len = TtyRead(0, buf_5, TERMINAL_MAX_LINE);
        char* info_str = "I am the child and I read in: ";
        TtyWrite(0, info_str, strlen(info_str) + 1);
        TtyWrite(0, buf_5, len);
        Exit(0);
    }
    else{
        Delay(15);
        char* final_text = "Parent exiting!\n";
        TtyWrite(0, final_text, strlen(final_text) + 1);
        Exit(0);
    }


    
}
