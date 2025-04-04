#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
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
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
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
// attempt to update caller's nice value to the first argument (new_nice)
// new_nice must be in [-20, 19]
// returns the nice value of the caller after attempting update
uint64 sys_nice(void)
{
	int nice;
	struct proc *mp;
	argint(0, &nice);
	mp = myproc();
	if(nice < 20 && nice > -21){
		mp->nice = nice;
	}
	return mp->nice;
}

// Update the first argument (runtime) to the callers runtime value
// Update the second arugment (vruntime) to the callers vruntime value
// Returns 0
uint64 sys_getruntime(void){
	uint64 runtime;
	uint64 vruntime;
	struct proc* mp;
	
	mp = myproc(); //get the current process
	
	//get the arguments
	argaddr(0, &runtime);
	argaddr(1, &vruntime);	

	//update the arguments to the new values
	copyout(mp->pagetable, runtime, (char*)&(mp->runtime), sizeof(mp->runtime));
	copyout(mp->pagetable, vruntime, (char*)&(mp->vruntime), sizeof(mp->vruntime));
	return 0;
}

// Attempts to start cfs by setting the cfs variable in proc.h (proc.c) to 1
// Updates cfs_sched_latency to the first arugment (latency)
// Updates cfs_max_timeslice to the second argument (max)
// Updates cfs_min_timeslice to the third argument (min)
// Returns 1
uint64 sys_startcfs(void){
	int late;
	int max;
	int min;
	
	argint(0, &late);
	argint(1, &max);
	argint(2, &min);
	
	cfs = 1;
	cfs_sched_latency = late;
	cfs_max_timeslice = max;
	cfs_min_timeslice = min;
	return 1;
}

// Stops cfs by setting cfs variable in proc.h (proc.c) to 0
// Returns 1
uint64 sys_stopcfs(void){
	cfs = 0;
	return 1;
}
