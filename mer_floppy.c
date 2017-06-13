#include "mer_floppy.h"
#include "mer_mm.h"

unsigned char digital_op_reg;
unsigned char main_stat_reg;
unsigned char digital_in_reg;
unsigned char config_ctrl_reg;

unsigned char op_code[16];

static void write_opcode(int);
static void write_controller(unsigned char);
static unsigned char read_controller();
static volatile char task_complete;
/*
 *	Floppy ISR
 */
void fdisk_isr()
{
	unsigned char st0 =0xff;
	mprintk("*FLoppy Interrupt", 17, 'c'); 
	/* send the Check Interrupt Status Command */
	op_code[0] = CMD_CHK_INTR_STATUS;
//	write_port_char(DIGITAL_OUT_REG, 28);
	write_opcode(1);
	st0 = read_controller();

	if (! (st0 & 0xc0))
		task_complete = 0x1;
//	write_port_char(DIGITAL_OUT_REG, 0x0);
	mprintk("*ST0:", 5, 'c');
	mprintk(&st0, 1, 'd');
}

/*
 * Function to read from controller's data register
 */

unsigned char read_controller()
{
	/* read main status reg and make sure that we can read */
	volatile unsigned char stat_reg;
	while(1) {
		stat_reg = read_port_char(MAIN_STAT_REG);
		if (stat_reg & 0x80) {
			mprintk("*read", 5, 'c');
			return read_port_char(DATA_REG);
		}	
	#ifdef GKT_DBG
		else {
			mprintk("*stat REG:", 10, 'c');
			mprintk(&stat_reg, 1, 'd');
		}
	#endif

	}
}

/* 
 * Function to write command and arguments to controller
 */
void write_controller(unsigned char opcode)
{
	/* read main status reg and make sure that we can write */
	volatile unsigned char stat_reg;
	while(1) {
		stat_reg = read_port_char(MAIN_STAT_REG);
		if (stat_reg & 0x80) {
			write_port_char(DATA_REG, opcode);
			mprintk("*wrote", 7, 'c');	
			return;
		}
	#ifdef GKT_DBG
		else {
			mprintk("*STAT REG:", 10, 'c');
			mprintk(&stat_reg, 1, 'd');
		}
	#endif
	}
}

void write_opcode(int opcode_len)
{
	int i;
	for (i=0; i<opcode_len; i++)
		write_controller(op_code[i]);
}

void generate_rs_opcode(char head, char track, char sector )
{
	if (head <0 || head > 1)
		return;
	/* FIXME: sanity check for track n sector also */

	op_code[0] = 0x46; /* no multi track op, no skip mode */
	op_code[1] = 0x0 | (head << 2);
	op_code[2] = track;
	op_code[3] = head;
	op_code[4] = sector;
	op_code[5] = 0x02; /* 512 bytes per sector */
	op_code[6] = 0x0f;
	op_code[7] = 0x2a;
	op_code[8] = 0x00; 
}
/*
 * DMA controller initialization
 *	1) disable and initialize the DMA controller
 *	2) set up DMA mode for channel 2
 *	3) write the buffer address to address and page registers
 *	4) load count value
 *	5) enable DMA controller
 */

void init_dma(unsigned int buffer)
{
	unsigned char addr1, addr2, addr3;
	/* write to DMA 1 CMD register */
	write_port_char(DMA1_CMD_REG, 0x14);
	/* write to DMA1 mode register, disable DMA */
	write_port_char(DMA1_MODE_REG, 0x56);
	
	/* load the address register, step 1, reset flip-flop */

	write_port_char(DMA1_FLIP_FLOP, 0xff);
	/* step 2, 3 and 4, split the address and write */
	addr1 = (unsigned char)(buffer & 0xff);
	addr2 = (unsigned char)((buffer >> 8) & 0xff);
	addr3 = (unsigned char)((buffer >> 16) & 0xff);

	write_port_char(DMA1_ADDR_REG_CH2, addr1);	
	write_port_char(DMA1_ADDR_REG_CH2, addr2);	
	write_port_char(DMA1_PAGE_REG_CH2, addr3);	
	
	write_port_char(DMA1_FLIP_FLOP, 0xff);

	/* load count register with 511, to transfer 512 bytes */
	write_port_char(DMA1_CNT_REG_CH2, 0xff);
	write_port_char(DMA1_CNT_REG_CH2, 0x1);

	/* release eventual channel masking */
	write_port_char(DMA1_CHANNEL_MASK, 0x02);

	/* enable DMA 1 */
	write_port_char(DMA1_CMD_REG, 0x10);
}
/*
 * Function to write a sector to floppy
 */

int write_sector(char head, char track, char sector, unsigned int buff)
{
	return 0;
}

/*
 *	Function to read a sector from floppy
 *
 *	1) program the DMA controller with address, cnt, direction etc
 *	2) Issue command to floppy controller
 *	3) hope to get an interrupt
 */

int read_sector(char head, char track, char sector, unsigned int buff)
{
	task_complete = 0x0;
	/* initialize DMA controller */
	init_dma(buff);	
	/* generate opcode */
	generate_rs_opcode(head, track, sector);

	/* write the opcode to controller */
//	write_port_char(DIGITAL_OUT_REG, 28);
	write_opcode(9);
	while(!task_complete) {
		mprintk("*WAIT", 5, 'c');
	}
	mclrscr();
	mprintk("*WAIT OVER", 10, 'c');
//	write_port_char(DIGITAL_OUT_REG, 0x0);
	return 0;
}

/*
 *	This is the initialization function for the floppy controller, the 
 *	BIOS would have done the init but we still do it
 *
 * 	Set the Digital Output Reg to 0x18, indicating Drive as A, DMA/IRQ
 *	enabled, Controller enabled and motor control for Drive A.
 *
 *	Set Config Ctrl Reg to 0x0, ie Data Transfer Rate = 500kbps whic
 *	is the optimum for 1.44 Mbyte drive
 */

int init_floppy()
{

	unsigned int read_buff = Get_Free_Pages(GFP_DMA, 0);
	
	if (!read_buff) {
		mprintk("*Get Free Pages Failed", 23, 'c');
		return;
	}
	read_buff = virt_to_phys(read_buff);
#ifdef GKT_DBG
	mprintk("*Get Free DMAes returned", 24, 'c');
	mprintk(&read_buff, 4, 'd');
#endif

	/* initialize fdisk controller */	
	
	write_port_char(DIGITAL_OUT_REG, 28);
	write_port_char(CONFIG_CTRL_REG, 0);
	
	/* hook interrupt 6 */
	request_irq(fdisk_isr, FDISK_IRQ);
	
#ifdef GKT_DBG	
	digital_op_reg = read_port_char(DIGITAL_OUT_REG);
	main_stat_reg = read_port_char(MAIN_STAT_REG);
	digital_in_reg = read_port_char(DIGITAL_IN_REG);
	config_ctrl_reg = read_port_char(CONFIG_CTRL_REG);
	
	mprintk("*Digital OP Reg:", 16, 'c');
	mprintk(&digital_op_reg, 1, 'd');
	mprintk("*Main Status Reg:", 17, 'c');
	mprintk(&main_stat_reg, 1, 'd');
	mprintk("*Digital in Reg :", 17, 'c');
	mprintk(&digital_in_reg, 1, 'd');
	mprintk("*Config Ctrl Reg:", 17, 'c');
	mprintk(&config_ctrl_reg, 1, 'd');
#endif

#ifndef GKT_DBG
	/* read head 0, track 0, sector 1 */
	if (read_sector(0, 0, 1, read_buff)) {
		mprintk("*read sector failed", 19, 'c');
		return  -1;
	}
	mprintk("*Read 1 sector", 14, 'c'); 
#endif
	return 0;
}

void setup_floppy()
{
	/* initialize the floppy controller */
	init_floppy();
}
