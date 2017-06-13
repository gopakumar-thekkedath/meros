/*
	ld script to make the meros kernel binary image
*/
OUTPUT_FORMAT("binary")
OUTPUT_ARCH(i386)
ENTRY(_start)
SECTIONS
{
	. = 0xc0000000 + 0x5000 ; /* meros kernel space begins at 3 GB */
	_text = .;   /* _text begins at 3GB */
	.text : {  	/* the code section */ 
		*(.text)
		}
	_etext = .;	/* end of code section */

	.rodata : {	/* read only data */
		*(.rodata)
		}	/* end of read only data */
	_sdata = .;
	.data : {	/* data */
		*(.data)
		}
	_edata = .;	/* end of data */

	_bss_start = .;	/* BSS start */
	.bss : {
		*(.bss)
		}
	_end = .;	/* end of kernel */

}
