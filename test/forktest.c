#include <yuser.h>



void
recurse(who, i)
     char *who;
     int i;
{
  char waste[1024];	/* use up stack space in the recursion */
  char *mem = (char *)malloc(2048); /* use up heap space */
  int j;

  for (j = 0; j < 1024; j++) 
    waste[j] = 'a';

  TracePrintf(1, "Target: %s %d\n", who, i);
  if (i == 0)
    {
      TracePrintf(1, "Done with recursion\n");
      return;
    }
  else
    recurse(who, i - 1);
}


int main(argc, argv)
     int argc;
     char *argv[];
{
  int pid;

  TracePrintf(0,"BEFORE\n");

  if ((pid = Fork()) == 0)
    {
      TracePrintf(0,"CHILD\n");
      recurse("child", 33);
      Exit(0);
    }
  else
    {
      Delay(4);
      TracePrintf(0,"PARENT: child pid = %d\n", pid);
      recurse("parent", 33);
    }

  Exit(0);
}
