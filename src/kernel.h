/**
 * Kevin Lin and Carter Sullivan
 * Yalnix
 * CS58 - Operating Systems 24W
 *
 * kernel.h
 * 
 */
#ifndef KERNEL_H
#define KERNEL_H

#include <hardware.h>
#include "globals.h"

void KernelStart(char * cmd_args[], unsigned int pmem_size,
                 UserContext *uctxt);
KernelContext *KCSwitch(KernelContext *kc_in, void *curr_pcb_p, void *next_pcb_p); // See 4.2
KernelContext *KCCopy(KernelContext *kc_in, void *curr_pcb_p, void *not_used); // See 4.3
int allocateFrame();
int deallocateFrame(int frame_index);
int SetKernelBrk(void * addr);
int runProcess();
pcb_t *initIdleProcess(UserContext *uctxt, char *args[], char *name);
pcb_t *initInitProcess(UserContext *uctxt, char *args[], char *name);
pcb_t *initProcess(UserContext *uctxt, char *args[], char *name);
pcb_t *createPCB(char* name);
int LoadProgram(char *name, char *args[], pcb_t *pcb);
void Checkpoint3TrapClock(UserContext *user_context);
int enqueueDelayQueue(Queue_t *queue, pcb_t* pcb);
int runFromReadyQueue();

#endif