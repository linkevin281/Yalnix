/**
 * Kevin Lin and Carter Sullivan
 * Yalnix
 * CS58 - Operating Systems 24W
 *
 * syscalls.h
 * 
 */

#ifndef YALNIX_H
#define YALNIX_H
#ifndef _KERNEL_H
#define _KERNEL_H

/* Creates a new process that is a copy of the calling process. */
int Y_Fork(void);
/* Replaces the currently running process with a new process image. */
int Y_Exec(char *filename, char *argv[]);
/* Terminates the calling process with exit code {status} */
int Y_Exit(int status);
/* Waits for a child process to terminate, returns the child's PID and stores status at ptr. */
int Y_Wait(int *status);
/* Returns the PID of the calling process. */
int Y_Getpid(void);
/* Changes the location of the program break. */
int Y_Brk(void *addr);
/* Delays the calling process for a specified number of clock ticks. */
int Y_Delay(int clock_ticks);
int Y_Ttyread(int tty_id, void *buf, int len);
int Y_Ttywrite(int tty_id, void *buf, int len);
int Y_Pipeinit(int *pipe_idp);
int Y_Piperead(int pipe_id, void *buf, int len);
int Y_Pipewrite(int pipe_id, void *buf, int len);
int Y_LockInit(int *lock_idp);
int Y_Acquire(int lock_id);
int Y_Release(int lock_id);
int Y_CvarInit(int *cvar_idp);
int Y_CvarSignal(int cvar_id);
int Y_CvarBroadcast(int cvar_id);
int Y_Cvarwait(int cvar_id, int lock_id);
int Y_Reclaim(int id);

#endif /* YALNIX_H */
#endif /* _KERNEL_H */