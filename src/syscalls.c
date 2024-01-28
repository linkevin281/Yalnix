/**
 * Kevin Lin and Carter Sullivan
 * Yalnix
 * CS58 - Operating Systems 24W
 *
 * syscalls.c
 *
 */

int Y_Fork()
{
}

int Y_Exec(char *filename, char *argv[])
{
}

int Y_Exit(int status)
{
}

int Y_Wait(int *status)
{
}

int Y_Getpid()
{
}

int Y_Brk(void *addr)
{
}

int Y_Delay(int clock_ticks)
{
}

int Y_Ttyread(int tty_id, void *buf, int len)
{
}

int Y_Ttywrite(int tty_id, void *buf, int len)
{
}

int Y_Pipeinit(int *pipe_idp)
{
}

int Y_Piperead(int pipe_id, void *buf, int len)
{
}

int Y_Pipewrite(int pipe_id, void *buf, int len)
{
}

int Y_LockInit(int *lock_idp)
{
}

int Y_Acquire(int lock_id)
{
}

int Y_Release(int lock_id)
{
}

int Y_CvarInit(int *cvar_idp)
{
}

int Y_CvarSignal(int cvar_id)
{
}

int Y_CvarBroadcast(int cvar_id)
{
}

int Y_Cvarwait(int cvar_id, int lock_id)
{
}

int Y_Reclaim(int id)
{
}
