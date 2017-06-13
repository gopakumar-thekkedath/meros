# include "mer_krnl.h"
# include "mer_intr.h"
/*
	Function : do_irq
*/
void do_irq(void)
{
	unsigned int irq_num =0;
//	asm("movl 12(%%esp), %0" : "=r"(irq_num):);
	/* this change as we use -fomit-frame-pointer */
	asm("movl 8(%%ebp), %0" : "=r"(irq_num):);
#ifndef DEBUG
	if (irq_num != 0 && irq_num !=1) {
		mprintk("*IRQ NUM :", 10, 'c');
		mprintk(&irq_num, 4, 'd');
	}
#endif
	/* check for validity of IRQ */
	if ( irq_num >15)
		return;
	/*  call the appropriate handler, if one is installed */ 
	if (dev_isr[irq_num])
		(*dev_isr[irq_num])();
#ifndef DEBUG
	else if (irq_num) {
		mprintk("*NO ISR HOOKED for:", 19, 'c');
		mprintk(&irq_num, 4, 'd');
	}
#endif	
}
/*
	Function : hook_intr
	Purpose:
		Load the IDT table with the address of individual ISRs, which
	will be triggered on an interrupt.
*/
void hook_intr()
{
	void  (*isr_ptr[MAX_ISR])();
	int i;
	isr_ptr[0] = isr_IRQ0;
//	isr_ptr[0] = isr_IRQ1;
	isr_ptr[1] = isr_IRQ1;
	isr_ptr[2] = isr_IRQ2;
	isr_ptr[3] = isr_IRQ3;
	isr_ptr[4] = isr_IRQ4;
	isr_ptr[5] = isr_IRQ5;
	isr_ptr[6] = isr_IRQ6;
	isr_ptr[7] = isr_IRQ7;
	isr_ptr[8] = isr_IRQ8;
	isr_ptr[9] = isr_IRQ9;
	isr_ptr[10] = isr_IRQ10;
	isr_ptr[11] = isr_IRQ11;
	isr_ptr[12] = isr_IRQ12;
	isr_ptr[13] = isr_IRQ13;
	isr_ptr[14] = isr_IRQ14;
	isr_ptr[15] = isr_IRQ15;
	unsigned int* idt_base = (unsigned int*)&meros_IDT;
	unsigned short lsbs;
	unsigned short msbs;
	unsigned int IDT_old;
#ifdef DEBUG
	mprintk("*ISR ADDR:", 10, 'c');
	mprintk(&isr_IRQ0, 4, 'd');
#endif
	/* disable processors INTR line */
	clear_intr();
	for (i=0; i<MAX_ISR; i++) {
		lsbs = (unsigned short)isr_ptr[i];
	//	lsbs = (unsigned short)keybrd_isr;
		msbs = (unsigned short)(
				(unsigned int)isr_ptr[i] >> 16);
	//	msbs = (unsigned short)(
	//			(unsigned int)keybrd_isr >> 16);
		*((unsigned short*)(idt_base + (HW_INTR_START + i)*2)) =	
							lsbs;
		IDT_old = *(idt_base + (HW_INTR_START + i)*2 + 1) ;
		IDT_old &=0x0000ffff;
		IDT_old |= (msbs << 16);
		*(idt_base + (HW_INTR_START +i)*2 + 1) =IDT_old;
		/* set the device specific ISR array to NULL */
		dev_isr[i] =NULL;
	}
	/* re enable interrupts */
	set_intr();
}

/*
	Function: request_irq
	Arguments:
		funptr : address of device specific ISR
		irq_num : the IRQ number for this ISR
	Purpose:
		The ISRs address should be recorded in the appropriate
		location of device specific ISR table. Interrupts needs
		to be disabled before the data structure is manipulated.
*/
void request_irq( void (*funptr)(), int irq_num)
{
	clear_intr();
	dev_isr[irq_num] = funptr;
#ifndef GKT_DBG
	mprintk("*request_irq for:", 17, 'c');
	mprintk(&irq_num, 4, 'd');
#endif
	set_intr();
}
