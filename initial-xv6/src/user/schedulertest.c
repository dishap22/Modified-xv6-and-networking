#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define NFORK 10
#define IO 5

int main()
{
  int n, pid;
  int wtime, rtime;
  int twtime = 0, trtime = 0;
  for (n = 0; n < NFORK; n++)
  {
    pid = fork();
    if (pid < 0)
      break;
    if (pid == 0)
    {
      if (n < IO)
      {
        #if defined LBS
        sleep(40);

        #else
        sleep(200); // IO bound processes
        #endif
      }
      else
      {
        for (volatile int i = 0; i < 1000000000; i++)
        {
        } // CPU bound process
      }
      printf("Process %d finished\n", n);
      exit(0);
    }
    #if defined LBS
    else {
      settickets(n * 2 + 1);
    }
    #endif
  }
  for (; n > 0; n--)
  {
    if (waitx(0, &wtime, &rtime) >= 0)
    {
      printf("\n%d ticket. rtime: %d, wtime: %d\n", 2*n + 1, rtime, wtime);
      trtime += rtime;
      twtime += wtime;
    }
  }
  printf("Average rtime %d,  wtime %d\n", trtime / NFORK, twtime / NFORK);
  exit(0);
}