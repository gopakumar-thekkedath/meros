
#include "mer_krnl.h"

extern void setup_floppy();
/*
	The arguments are start and end physical address of kernel.
	From this, find the corresponding PFNs and mark them as 1.
*/

inline void setup_devices(void)
{
	/* keyboard stuff */
	setup_keyboard();
	/* timer stuff */
	setup_timer();
	/* floppy */
	setup_floppy();
}

inline void zero_fill(void *start, unsigned int bytes)
{
	int i;
	for (i=0; i<bytes; i++)
		*((char*)start + i) = 0;
}
/*
 *	Function: mclrscr()
 *		Clears the screen
*/
void mclrscr()
{
	int i;
	for (i=0; i<4000; i+=2) {
		*(DISP_PTR + i) = ' ';
		*(DISP_PTR + i + 1) = 0x0;
	}
	/* reset the cursor location to zero */
	*CUR_PTR = 0x0;
	mprintk("meros>", 6, 'c' );
}
/*
 *	Function: mer_start
 *		The first kernel function to get invoked from assembly
*/
extern char _text;
extern char _etext;
extern char _sdata;
extern char _edata;
extern char _end;
extern unsigned int meros_pg_dir;
extern unsigned int meros_pg0;
extern unsigned int test_value;
unsigned int *pgdir_start;

void mer_start()
{
	char *mem_str = "Total Physical Memory :";
	char *hd0_yes = "*HD0 Present";
	char *hd1_yes = "*HD1 Present";
	char *mouse_yes = "*Mouse Detected";
	char *kernel_start = "*KERNEL STart :";
	char *kernel_cend = "*KERNEL Code End :";
/*	
	char *kernel_dstart = "*KERNEL Data Start :";
	char *kernel_dend = "*KERNEL Data End :";
	char *kernel_bstart = "*KERNEL BSS Start :";
	char *kernel_bend = "*KERNEL BSS End :";
*/
	char *kernel_end = "*KERNEL End :";
	char *bitmap_start = "*bitmap start :";
	char *test	= "*test :";

	unsigned int mer_code_start = b_mem_mgr.kernel_start =
				(unsigned int) &_text;
	unsigned int mer_rodata_start;
	unsigned int mer_code_end = mer_rodata_start = (unsigned int)&_etext;
	unsigned int mer_data_start;
	unsigned int mer_rodata_end = mer_data_start = (unsigned int)&_sdata;
	unsigned int mer_data_end = (unsigned int)&_edata;
	unsigned int mer_krnl_end = b_mem_mgr.kernel_end =
					 (unsigned int)&_end;
	unsigned int krnl_start_phy;
	unsigned int krnl_end_phy;
//	unsigned int next_free_page;
	int i;
	/* copy the system configuration information from
		0x7c00 to 0x0 
	*/

	for (i=0; i<SEC_SIZE/4 ; i++)
		*((unsigned int*)ZERO_PAGE) = *((unsigned int*)CONFIG_PTR);
	b_mem_mgr.mem_size = *(MEM_PTR);
	//b_mem_mgr.mem_size <<= 10;

	unsigned char mouse = *(MOUSE_PTR);
	unsigned short hdisk = *(HD_PTR);
	mclrscr();
#ifdef GKT_DBG
	mprintk(mem_str, 23, 'c');
	mprintk(&b_mem_mgr.mem_size, 4, 'd');
	if (mouse)
		mprintk(mouse_yes, 14, 'c');
	if (*((char*)&hdisk))
		mprintk(hd0_yes, 11, 'c');
	if (*((char*)&hdisk + 1))
		mprintk(hd1_yes, 11, 'c');

	mprintk(kernel_start, 14, 'c');
	mprintk(&b_mem_mgr.kernel_start, 4, 'd');
	
	mprintk(kernel_start, 14, 'c');
	mprintk(&mer_code_start, 4, 'd');
	mprintk(kernel_end, 11, 'c');
	mprintk(&mer_krnl_end, 4, 'd');
#endif
	/* zero fill the mem mgr structure */
//	zero_fill(&b_mem_mgr, sizeof(b_mem_mgr));
	/* get the starting address of bitmap and align it to 4 bytes */
	b_mem_mgr.bitmap = (unsigned char *)(mer_krnl_end + 1) + 3;
	((unsigned long)b_mem_mgr.bitmap)&= ~3;
	/* find the total number of pages in this machine*/
	b_mem_mgr.total_pages = b_mem_mgr.mem_size >> 2;
	/* find the number of bytes required for our bit map */
	b_mem_mgr.bitmap_size = (b_mem_mgr.total_pages + 7)/8;
	
	/* initially zero fill the bitmap area */
	zero_fill(b_mem_mgr.bitmap, b_mem_mgr.bitmap_size);

	/* mark the page corresponding to the first physical page
		as reserved as it contains the system configuration
		data
	*/
	
	reserve_pages(0, PAGE_SIZE - 1);
	/* 
	now mark the pages representing kernel and memory hole
	as reserved.
	*/
	krnl_start_phy = virt_to_phys(b_mem_mgr.kernel_start);
	krnl_end_phy = virt_to_phys(b_mem_mgr.kernel_end);
	
	reserve_pages(krnl_start_phy,
		      krnl_end_phy + 
	             (b_mem_mgr.bitmap - b_mem_mgr.kernel_end)+
		      b_mem_mgr.bitmap_size);
	/* also reserve the memory hole (640KB - 1MB) */
	reserve_pages(655360, 1048576);
	/*
	 FIXME: This is to reserve mem_stup code as the GDT is there
		later move the GDT to mer_prot and mark this page as
		reserved.
	*/		
	reserve_pages(0x1000, 0x1000+ 1024-1);
	//reserve_pages(0x8e00, 0x8e00+ 512-1);
	/*
	 reserve 2 pages for kernel stack.
	*/
	reserve_pages(581632, 589824);
	pgdir_start = (unsigned int)&meros_pg_dir ;
	test_value = *pgdir_start;
	/* represent the memory size in bytes */
	b_mem_mgr.mem_size <<= 10;
//	mclrscr();
#ifdef GKT_DBG
	mprintk("*memsize in bytes :", 19, 'c');
	mprintk(&b_mem_mgr.mem_size, 4, 'd');
#endif
	/* set the page tables to map the entire physical memory
		from virtual address 0xC0000000 ie 768th entry of PGD
	*/
	setup_pagetables(pgdir_start, b_mem_mgr.mem_size);
#ifdef GKT_DBG
	mprintk("*setup_pagetable", 16, 'c');
#endif
	/* hook  all the interrupts*/
	hook_intr();
	
	/*
		Get ready for the second stage of memory managment. This
	one will have node, zones and pages and the buddy allocator. 
		Allocate memory for mem_map array, initialize it. Also
	initialize the node and various zones. In each zone, initialize
	the free_area array, allocate memory for the bitmap pointers.
	*/
	start_zone_mgmt();	
	/*
		Let the buddy allocator build free lists using the
	unallocated pages.
	*/
	mem_init();

	/*Time for the initial slab manager setup */
	/*9/12/04, as we are currently implementing the slab manager in
		out style, below function is empty for now */
	kmem_cache_init();
	
	/* initial set up for kmalloc */
	kmem_cache_sizes_init();
#ifdef GKT_DBG
{
	unsigned int test;
	test =	kmalloc(12, 0);
	mprintk("*KMALLOC RET ADDR:", 18, 'c');
	mprintk(&test, 4, 'd');
	kfree(test);
}
#endif
	/* setup all devices */
	setup_devices();
	
#ifdef GKT_DBG
	/* setup initial task switching */
	setup_task();
#endif
	task1();
}
