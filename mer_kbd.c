/*
	mer_kbd.c
*/
#include "mer_krnl.h"
#include "mer_kbd.h"

#ifndef GKT_DBG
unsigned int num_key_stroke =0;
#endif
//void keyboard_tasklet()
void keyboard_isr()
{
	unsigned char scan_code =read_port_char(0x60);
	unsigned char ascii;
#ifndef GKT_DBG
	volatile unsigned char *video = 0xc0000000 + 0xb8000;
	num_key_stroke++;
#endif
	if (scan_code < MAXKEY) {
		ascii = keytab[scan_code];
		if (ascii)
			if (ascii == 10) /* enter */
				mprintk("*", 1, 'c');
			else
				mprintk(&ascii, 1 , 'c' );
	}
	
}
inline void hook_interrupt()
{
	request_irq(keyboard_isr, 1);
#if 0
	unsigned int* idt_base = (unsigned int*)&meros_IDT;
	unsigned short lsbs = (unsigned short)keybrd_isr;
	unsigned short msbs = (unsigned short)(
				(unsigned int)keybrd_isr >>16);
	unsigned int IDT_old;
	int i;
	unsigned int port_val =0;
	unsigned int master_imr = 0;
	unsigned int slave_imr = 0;
	
	/* set up the new ISR address*/

	*((unsigned short*)(idt_base + KEYBD_VECT*2)) =  lsbs;
	IDT_old = *(idt_base + KEYBD_VECT*2 + 1) ;
	IDT_old &=0x0000ffff;
	IDT_old |= (msbs << 16);
	*(idt_base + KEYBD_VECT*2 + 1) =IDT_old;
#endif	
}
void setup_key_map(void)
{
	keytab[2] = 49;
	keytab[3] = 50;
	keytab[4] = 51;
	keytab[5] = 52;
	keytab[6] = 53;
	keytab[7] = 54;
	keytab[8] = 55;
	keytab[9] = 56;
	keytab[10] = 57;
	keytab[11] = 58;
	
	keytab[16] = 113;
	keytab[17] = 119;
	keytab[18] = 101;
	keytab[19] = 114;
	keytab[20] = 116;
	keytab[21] = 121;
	keytab[22] = 117;
	keytab[23] = 105;
	keytab[24] = 111;
	keytab[25] = 112;

	keytab[30] = 97;
	keytab[31] = 115;
	keytab[32] = 100;
	keytab[33] = 102;
	keytab[34] = 103;
	keytab[35] = 104;
	keytab[36] = 106;
	keytab[37] = 107;
	keytab[38] = 108;
	
	keytab[44] = 122;
	keytab[45] = 120;
	keytab[46] = 99;
	keytab[47] = 118;
	keytab[48] = 98;
	keytab[49] = 110;
	keytab[50] = 109;
	
	keytab[57] = 32; /* space */
	keytab[28] = 10; /* enter */
}
void setup_keyboard(void)
{
	/* set up the key map */
	setup_key_map();

	/* enable keyboard */	
	enable_kbd();
	/* hook the keyboard interrupt */
	hook_interrupt();
}
