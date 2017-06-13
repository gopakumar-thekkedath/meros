/* Floppy Controller Registers */

#define FLOPPY_BASE		0x3f0
#define STAT_REG_A		0x3f1
#define STAT_REG_B		0x3f1
#define DIGITAL_OUT_REG		0x3f2
#define MAIN_STAT_REG		0x3f4
#define DATA_RATE_SEL_REG	0x3f4
#define DATA_REG		0x3f5
#define DIGITAL_IN_REG		0x3f7
#define CONFIG_CTRL_REG		0x3f7

/* DMA controller registers */

#define DMA1_ADDR_REG_CH2	0x04
#define DMA1_CNT_REG_CH2	0x05
#define DMA1_STAT_REG		0x08
#define DMA1_CMD_REG		0x08
#define DMA1_REQ_REG		0x09
#define DMA1_CHANNEL_MASK	0x0a
#define DMA1_MODE_REG		0x0b
#define DMA1_FLIP_FLOP		0x0c
#define DMA1_PAGE_REG_CH2	0x81
/* */
#define FDISK_IRQ		0x6
#define DMA_CHAN		0x2


/* Floppy controller commands */

# define CMD_CHK_INTR_STATUS	0x8
