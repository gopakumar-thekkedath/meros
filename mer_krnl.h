//#define DEBUG

#define SEC_SIZE	(512)
#define ZERO_PAGE	(0xC0000000)
//#define CONFIG_PTR	((0xC0000000) + (0x7c00))
#define CONFIG_PTR	((0xC0000000) + (0x1000))
#define DISP_PTR	((volatile unsigned char*)0xC00B8000)
#define MEM_PTR		((unsigned int*)ZERO_PAGE + 0x0)
#define CUR_PTR		((unsigned short*)ZERO_PAGE + 4)
#define MOUSE_PTR	((unsigned char*)ZERO_PAGE + 0x28)
#define HD_PTR		((unsigned short*)ZERO_PAGE +0x29)
#define GB_3		(0xC0000000)
#define PAGE_SIZE	(1 << 12)
#define PAGE_SHIFT	12
#define PGD_SIZE	1024
#define PT_SIZE		1024
#define MAX_ISR		15
#define HW_INTR_START   32
#define	NULL		0x0
#define KEYBD_VECT	33

/* Page Table Specific Flags */
#define PRESENT		1
#define CACHE_DISABLE   (1 << 4)
extern void flush_tlb(void);
extern unsigned int read_port_char(unsigned int port);
extern void write_port_char(unsigned int port, unsigned int value);
extern unsigned int meros_IDT;
extern void hook_intr();
extern void request_irq( void (*funptr)(), int irq_num);

extern void enable_kbd(void);
extern void keybrd_isr(void);

extern void isr_IRQ0(void);
extern void isr_IRQ1(void);
extern void isr_IRQ2(void);
extern void isr_IRQ3(void);
extern void isr_IRQ4(void);
extern void isr_IRQ5(void);
extern void isr_IRQ6(void);
extern void isr_IRQ7(void);
extern void isr_IRQ8(void);
extern void isr_IRQ9(void);
extern void isr_IRQ10(void);
extern void isr_IRQ11(void);
extern void isr_IRQ12(void);
extern void isr_IRQ13(void);
extern void isr_IRQ14(void);
extern void isr_IRQ15(void);
struct boot_mem_mgr {
	unsigned char	*bitmap;     /* starting byte of bitmap */
	unsigned int	bitmap_size; /* total bitmap size */
	unsigned int	total_pages; /* total number of pages in phy memory */
	unsigned int	kernel_start; /* kernel start address */
	unsigned int 	kernel_end;  /* kernel end address */
	unsigned int 	mem_size;
}b_mem_mgr;

extern void task1(void);
extern void kmem_cache_init(void);
