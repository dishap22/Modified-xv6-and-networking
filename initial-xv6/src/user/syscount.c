#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int get_syscall_number(unsigned int mask) {
    int position = 0;
    while ((mask & 1) == 0) {
        mask >>= 1;
        position++;
    }
    return position;
}

int main(int argc, char *argv[])
{
  if(argc < 3) {
    fprintf(2, "Usage: syscount <mask> command [args]\n");
    exit(1);
  }

  int mask = atoi(argv[1]);
  int val = get_syscall_number(mask); 
  if (val == 0 || mask == 0) {
    fprintf(2, "Usage: syscount <mask> command [args]\n");
    exit(1);
  }

  int pid = fork();
  if (pid < 0) {
    fprintf(2, "fork failed\n");
    exit(1);
  } else if (pid == 0) {
    // executing the command in the child process
    exec(argv[2], &argv[2]);
    fprintf(2, "exec %s failed\n", argv[2]);
    exit(1);
  } else {
    // wait for child to execute and then get syscall count 
    wait(0);
    int count = getsyscount(mask);
    char *syscall_names[] = { "invalid",
      "fork", "exit", "wait", "pipe", "read", "kill", "exec",
      "fstat", "chdir", "dup", "getpid", "sbrk", "sleep", "uptime",
      "open", "write", "mknod", "unlink", "link", "mkdir", "close",
      "wait", "getsyscount"
    };
    printf("PID %d called %s %d times\n", pid, syscall_names[val], count);
  }
  exit(0);
}