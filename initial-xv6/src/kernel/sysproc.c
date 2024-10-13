#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "syscall.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n)
  {
    if (killed(myproc()))
    {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_waitx(void)
{
  uint64 addr, addr1, addr2;
  uint wtime, rtime;
  argaddr(0, &addr);
  argaddr(1, &addr1); // user virtual memory
  argaddr(2, &addr2);
  int ret = waitx(addr, &wtime, &rtime);
  struct proc *p = myproc();
  if (copyout(p->pagetable, addr1, (char *)&wtime, sizeof(int)) < 0)
    return -1;
  if (copyout(p->pagetable, addr2, (char *)&rtime, sizeof(int)) < 0)
    return -1;
  return ret;
}

int get_syscall_number(unsigned int mask) {
    int position = 0;
    while ((mask & 1) == 0) {
        mask >>= 1;
        position++;
    }
    return position;
}

uint64
sys_getSysCount(void) {
  int mask;
  argint(0, &mask);
  
  // making sure only one bit is set in the mask, i.e. that it's a valid mask
  if (mask == 0 || (mask & (mask - 1)) != 0)
    return -1;
  
  int syscall_num = get_syscall_number(mask);
  if (syscall_num < 1 || syscall_num > SYS_getsyscount)
    return -1;

  return myproc()->syscall_count[syscall_num]; // get and return count of respective syscall from current process
}

uint64 
sys_sigalarm(void) {
  int interval;
  argint(0, &interval);

  uint64 alarm_handler;
  argaddr(1, &alarm_handler);

  struct proc *p = myproc();

  p->current_ticks = 0;
  p->interval = interval;
  p->alarm_handler = alarm_handler;
  p->alarm_status = 0;

  return 0;
}

uint64
sys_sigreturn(void) {
  struct proc *p = myproc();

  // restoring trapframe to the original one
  memmove(p->trapframe, p->alarm_tf, sizeof(*p->trapframe));
  kfree((char*)p->alarm_tf);
  p-> alarm_status = 0;
  usertrapret();
  return 0;
}

uint64
sys_settickets(void) {
  int tickets; 
  argint(0, &tickets);
  myproc()->tickets = tickets;
  return 0;
}