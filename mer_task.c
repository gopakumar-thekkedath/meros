/*
	mer_task.c
*/
#ifdef GKT_DBG

#include "mer_task.h"
#include "mer_krnl.h"

#ifdef GKT_DBG
extern unsigned int num_key_stroke;
#endif
/*
	task1
*/
unsigned long task2_start_addr;
extern unsigned long restore_all;
extern void schedule();
extern void dont_schedule();
extern void trial();

task_struct *tsk1, *tsk2;
static int toggle =0;

task_struct *find_best_process(void)
{
	if (toggle ==0) {
		toggle =1;
		return tsk2;
	} else
	{
		toggle =0;
		return tsk1;
	}
}
void schedule()
{	
	task_struct *next_task;
	unsigned int cur_stack_ptr;
	unsigned int cur_task;
#ifdef TASK_DEBUG	
	mprintk("*schedule", 9,'c');
#endif
	/* save the current stack pointer in current->ESP*/
	asm("movl %%esp, %0":"=r"(cur_stack_ptr):);
#ifdef TASK_DEBUG
	mprintk("*Current Stack Ptr:", 19, 'c');
	mprintk(&cur_stack_ptr, 4, 'd');
#endif
	cur_task = cur_stack_ptr & 0xfffff000;
	/*
		As soon as we entered this function, the ESP would have
		got stored in EBP	
	*/
	asm("movl %%ebp, %0":"=r"(((task_struct*)cur_task)->ESP): );
#ifdef TASK_DEBUG
	mprintk("*Current task ESP:", 18, 'c');
	mprintk(&((task_struct*)cur_task)->ESP, 4, 'd');
#endif
	
	/* select the best candidate to schedule in */
	next_task = find_best_process();
	/* swap the stacks, ie make the selected process's stack
		the current kernel stack, so that on return from this
		function restore_all will result in the selected task
		being called.
	
		Stack switching is acheived by loading EBP with 
		next_task->ESP. The idea is, when the processor calls
		'leave' instruction (this is called before ret), the
		sequence of instructions are

			movl %ebp, %esp
			popl %ebp

		ie the ESP gets loaded with the content of EBP, then
		4 byte from the stack is popped out to EBP. 
			
	*/
#ifdef TASK_DEBUG
	mprintk("*Next task:", 11, 'c');
	mprintk(&next_task, 4, 'd');
	mprintk("*Next tasks ESP:", 16, 'c');
	mprintk(&next_task->ESP, 4, 'd');
#endif
#ifdef STACK_DEBUG	
	if (next_task == tsk1) {
		mprintk("*HALT", 5, 'c');		
		asm("cli");
		while(1);
	}
#endif
	asm("movl %0, %%ebp": : "r"(next_task->ESP));
//	asm("movl %%ebp, %%esp");
//	asm("popl %%ebp");
}
void dont_schedule()
{
#ifdef DEBUG
	mprintk("*dont_schedule",14, 'c');
#endif
}

#ifdef GKT_DBG
char find_num_key_stroke()
{
	char char_val[] = {'0', '1', '2', '3', '4', '5', '6' ,'7' , '8', '9'};
	if (num_key_stroke <= 9)
		return char_val[num_key_stroke];
	else
		return '*';
}
#endif

void task1(void)
{
#ifdef GKT_DBG
	char c;
#endif
	volatile unsigned char *video = 0xc0000000 + 0xb8000;
	
	while(1) {
	#ifdef GKT_DBG
		c =  find_num_key_stroke();
		*video = c;
	#else
		*video = 'M';
	#endif
		}
}	
/*	task2
*/
void task2(void)
{
#ifdef GKT_DBG
	char c;
#endif
	volatile unsigned char *video = 0xc0000000 + 0xb8000;
	while(1)  {
	#ifdef GKT_DBG
		c =  find_num_key_stroke();
		*video = c;
	#else
		*video = 'm';
	#endif
	}
}
task_struct* set_task_struct(unsigned int next_free_pg)
{
	unsigned int phy_addr;
	phy_addr = page_to_byte(next_free_pg);
	return (task_struct*) phys_to_virt(phy_addr);
}
/*
	Function: setup_t2_stack
	purpose: sets up the kernel stack for task2
*/
void setup_t2_stack(unsigned long tsk2_stack_start )
{
/* 
	now tsk2_stack is at the bottom of the stack, we need to store
	13 register values + 1 IRQ number in stacke ie 14*4 = 56 bytes.

	17/08/04
	Also if and when task2 is selected for scheduling, it will be
	from the schedule() function and we need to return to the low 
	level time ISR, "isr_IRQ0' mer_prot.asm, so also in task2's stack 
	we should have the return address of isr_IRQ0 also in the stack.
	so that make it 15 
 
	so we advance tsk2_stack by 60 (15*5) bytes to perform this filling
*/
	unsigned long *tsk2_stack_cur = (unsigned long*)(tsk2_stack_start-64);
	
	*(tsk2_stack_cur + 0)  = 0x11 ;// Dummy value to hold EBP
	*(tsk2_stack_cur + 1)  = trial ;// enter IRQ0's ISR
//	*(tsk2_stack_cur + 1)  = restore_all ;// enter IRQ0's ISR
	*(tsk2_stack_cur + 2)  = 0xb ;// IRQ number
	*(tsk2_stack_cur + 3)  = 15 ;// EBX
	*(tsk2_stack_cur + 4)  = 0x2 ;// ECX
	*(tsk2_stack_cur + 5)  = 0x3 ;// EDX
	*(tsk2_stack_cur + 6)  = 0x4 ;//ESI
	*(tsk2_stack_cur + 7)  = 0x5 ;//EDI
	*(tsk2_stack_cur + 8)  = 0x6 ;//EBP
	*(tsk2_stack_cur + 9)  = 0x7 ;//ECX	
	*(tsk2_stack_cur + 10)  = 0x8 ;//EAX
	*(tsk2_stack_cur + 11) = 0x18 ;//DS and ES
	*(tsk2_stack_cur + 12) = 0x18 ;// ES
	
	*(tsk2_stack_cur + 13) = task2 ;//EIP 
	
	*(tsk2_stack_cur + 14) = 0x10 ;//CS
	*(tsk2_stack_cur + 15) = 0x286 ;//EFLAGS, IF, SF, PF
}
void inline setup_t2_task_struct(task_struct *tsk2)
{
	tsk2->need_resched = 0; 
	/*
		when task2 is selected for scheduling, we will be
		reloading the ESP with task2's task_struct->ESP. After
		this the kernel will be executing in task2's kernel mode
		stack and there we expect to find the return address to
		timer ISR in mer_prot.asm. We have already set up the stack
		with some dummy register values and the return address to
		timer ISR. 

		Load tsk2->ESP with the return address to isr_IRQ0
	*/
	tsk2->ESP = (unsigned long)tsk2 + 4096 - 64;
#ifdef TASK_DEBUG	
	mprintk("*tsk2:", 6, 'c');
	mprintk(&tsk2, 4, 'd');
	mprintk("*tsk2->ESP:", 11, 'c');
	mprintk(&tsk2->ESP, 4, 'd');
#endif
}
void inline setup_t1_task_struct(task_struct *tsk1)
{
	tsk1->need_resched = 0; 
	tsk1->ESP = 0;
}
void setup_task(void)
{
	unsigned int next_free_page;
	unsigned long tsk1_stack_start;
	unsigned int eflags;
	/* get free pages for task1  */
	next_free_page = get_free_page();
	if (next_free_page <0)
		goto quit;
	/* get a page to point the task_struct */
	tsk1 = set_task_struct(next_free_page);
	/* initialize the task_struct for task1 */
	setup_t1_task_struct(tsk1);
	/* same for task2 */
	next_free_page = get_free_page();
	if (next_free_page <0)
		goto quit;
	tsk2 = set_task_struct(next_free_page);
	task2_start_addr = (unsigned long)tsk2 + 4096;
#ifdef TASK_DEBUG
	mprintk("*Task1 addr :", 13, 'c');
	mprintk(&tsk1, 4, 'd');
	mprintk("*Task2 addr :", 13, 'c');
	mprintk(&tsk2, 4, 'd');
#endif

/*
	Move the kernel stack pointer to tsk1+4kb-1
*/
	tsk1_stack_start = ((char*)tsk1)+4096;
	asm("movl %0, %%esp": : "r"(tsk1_stack_start));
#ifdef DISPLAY_EFLAGS	
	asm("pushfl");
	asm("popl %0":"=r"(eflags):);
	mprintk("*EFLAGS:", 8, 'c');
	mprintk(&eflags, 4, 'd');
#endif
	
/*
	setup the task2's stack, this is important as when task2 is
	selected for scheduling, we reload the ESP with task2+4096, so
	the ISR expects to find the relevent registers in stack.
*/
	/* invoke task1(), it should use the new stack*/

	/* set the task2's stack up with dummy register values and all */
	setup_t2_stack((unsigned long)tsk2 + 4096);
	setup_t2_task_struct(tsk2);
	task1();
quit:
	return;
	
}


#else
void task1()
{
	while(1);
}
#endif
