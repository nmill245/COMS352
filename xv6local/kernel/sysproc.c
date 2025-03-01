#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

struct spinlock t_wait_lock;

uint64 sys_exit(void) {
  int n;
  argint(0, &n);
  exit(n);
  return 0; // not reached
}

uint64 sys_getpid(void) { return myproc()->pid; }

uint64 sys_fork(void) { return fork(); }

uint64 sys_wait(void) {
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64 sys_sbrk(void) {
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

uint64 sys_sleep(void) {
  int n;
  uint ticks0;

  argint(0, &n);
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (killed(myproc())) {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64 sys_kill(void) {
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64 sys_uptime(void) {
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
uint64 sys_getppid(void) {
//get the process->parent->id
uint64 id;
  acquire(&t_wait_lock);
id = myproc()->parent->pid;
  release(&t_wait_lock);
	return id;
}
uint64 sys_getcpids(void) {
	int n;
    int i = 0;
	uint64 arr;
	struct proc* p;
	struct proc* mp;

	//get arguments
	argaddr(0, &arr);
	argint(1, &n);
	mp = myproc();


  acquire(&t_wait_lock);
  for(p = proc; p < &proc[NPROC]; p++) {
	if(p->parent == mp){
		copyout(mp->pagetable, arr+sizeof(i)*i, (char *)&(p->pid), sizeof(p->pid));
	i++;
}
}
  release(&t_wait_lock);
  return i;
}
uint64 sys_getpaddr(void) {
	uint64 vaddr;
	uint64 paddr;
	argaddr(0, &vaddr);
	pte_t *pte;
	pte = walk(myproc()->pagetable, vaddr, 0);
	if(pte!=0 && (*pte & PTE_V)){
		paddr = PTE2PA(*pte)|(vaddr & 0xFFF);
		return paddr;
	} else{
	  return 0;
	}
}
uint64 sys_gettraphistory(void) {
  // TODO
  return 0;
}
