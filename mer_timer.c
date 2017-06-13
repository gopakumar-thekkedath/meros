/*
	mer_timer.c
*/
#include "mer_krnl.h"
#include "mer_task.h"

extern unsigned long task2_start_addr;
/*
	high level timer interrupt handler
*/
void timer_isr()
{

#ifndef GKT_DBG
	static int sched_counter =0;
	unsigned int cur_stack_ptr;
	unsigned int cur_task;
	unsigned int need_resched;
	/*
	TODO: 
	 Get the current task_struct using the kernel mode ESP
	*/
#ifdef DEBGUG
	mprintk("timer", 5, 'c');
#endif
	asm("movl %%esp, %0": "=r"(cur_stack_ptr):);
	cur_task = cur_stack_ptr&0xfffff000;
	need_resched = ((task_struct*)cur_task)->need_resched;
#ifdef DEBGUG
	mprintk("task1 need_resched:", 19, 'c');
	mprintk(&need_resched, 4, 'd');
#endif
	if (sched_counter++ == 5) {
	#ifdef DEBUG
		mprintk("*Time for switch", 16, 'c');
	#endif
		 sched_counter = 0;
		/*
		TODO: 
		 set the need_resched flag in current task_struct
		*/
		((task_struct*)cur_task)->need_resched = 1;
			
	}
	/* 
		TODO:
		 reload EBX register with the starting address of task_struct 
		this is not necessary as we will use the stack pointer to
		get the current task_struct in low level ISR also.
	*/

#endif
}
/*
	routine to attach a timer ISR
*/
inline void hook_timer_interrupt()
{
	request_irq(timer_isr, 0);
}
void setup_timer(void)
{
	/* attach an ISR */
	hook_timer_interrupt();
}
