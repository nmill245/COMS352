#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

//aquire a waitlock in this file
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
	int count = 0;
	uint64 arr;
	struct proc* p;
	struct proc* mp;

	//get arguments
	argaddr(0, &arr);
	argint(1, &n);

	//get current process
	mp = myproc();

	//aquire the lock to access parents of all processes
  acquire(&t_wait_lock);
	
	//loop through every process checking if child
  for(p = proc; p < &proc[NPROC] && count < n; p++) {
		if(p->parent == mp){
			//append to the return array
			copyout(mp->pagetable, arr+sizeof(count)*count, (char *)&(p->pid), sizeof(p->pid));
			count++;
		}
	}
	//release lock
  release(&t_wait_lock);
  return count;
}

uint64 sys_getpaddr(void) {
	//create the virtual and actual address
	uint64 vaddr;
	uint64 paddr;

	//recieve the pointer to the vaddr
	argaddr(0, &vaddr);
	
	//get the actual address space from the virtual address
	pte_t *pte;
	pte = walk(myproc()->pagetable, vaddr, 0);

	//check if valid actual address
	if(pte!=0 && (*pte & PTE_V)){
		//parse physical address with virtual address
		paddr = PTE2PA(*pte)|(vaddr & 0xFFF);
		return paddr;
	} else{
		//when not valid return 0
	  return 0;
	}
}

uint64 sys_gettraphistory(void) {
	//declare the pointers to be used for return values
	uint64 totaladdr, saddr, daddr, timeaddr;
	//find the current process
	struct proc* mp;
	mp = myproc();

	//get arguments
	argaddr(0, &totaladdr);
	argaddr(1, &saddr);
	argaddr(2, &daddr);
	argaddr(3, &timeaddr);

	//upload arguments
	copyout(mp->pagetable, totaladdr,(char *)&(mp->trapcount), sizeof(mp->trapcount));
	copyout(mp->pagetable, saddr,(char *)&(mp->syscallcount), sizeof(mp->syscallcount));
	copyout(mp->pagetable, daddr,(char *)&(mp->devintcount), sizeof(mp->devintcount));
	copyout(mp->pagetable, timeaddr,(char *)&(mp->timerintcount), sizeof(mp->timerintcount));
  return 0;
}
