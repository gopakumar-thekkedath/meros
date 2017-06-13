
#	File		: Makefile
#	Version		: 0.0.1
#	Edited		: 18/06/04
#	Description	: Makefile for creation of 'meros' kernel
#

PROJECT= meros
OBJS = mer_prot.o mer_krnl.o mer_print.o mer_mm.o mer_kbd.o mer_intr.o mer_task.o mer_timer.o mer_buddy.o mer_slab.o mer_floppy.o
CFLAGS = -ffreestanding
#CFLAGS += fomit-frame-pointer
BACKUP = /home/gop/meros_design/BKUP
DATE = `date +%Y%m%d%H%M`
BINARY = binary
GCC = /mnt/hdisk/usr/bin/gcc

# Remove the comment from below statement if you want debug option
#CFLAGS += -DDEBUG

mer_krnl.bin : $(OBJS)
	ld -o mer_krnl.bin  --script=mer_link.lds $(OBJS)
	mv -f mer_krnl.bin binary
	@echo "Finished making mer_krnl.bin"
mer_prot.o : mer_prot.asm
	nasm -f elf -o mer_prot.o mer_prot.asm
mer_slab.o: mer_slab.c mer_slab.h
	$(GCC) $(CFLAGS) -c -o mer_slab.o mer_slab.c
mer_buddy.o: mer_buddy.c mer_krnl.h
	$(GCC) $(CFLAGS) -c -o mer_buddy.o mer_buddy.c

mer_krnl.o : mer_krnl.c mer_krnl.h
	$(GCC) $(CFLAGS) -c -o mer_krnl.o mer_krnl.c
mer_print.o : mer_print.c mer_krnl.h
	$(GCC) $(CFLAGS) -c -o mer_print.o mer_print.c
mer_mm.o    : mer_mm.c mer_krnl.h
	$(GCC) $(CFLAGS) -c -o mer_mm.o mer_mm.c
mer_kbd.o   : mer_kbd.c  mer_krnl.h mer_kbd.h 
	$(GCC) $(CFLAGS) -c -o mer_kbd.o mer_kbd.c
mer_intr.o  : mer_intr.c mer_krnl.h mer_intr.h
	$(GCC) $(CFLAGS) -c -o mer_intr.o mer_intr.c
mer_task.o  : mer_task.c mer_task.h
	$(GCC) $(CFLAGS) -c -o mer_task.o mer_task.c
mer_timer.o : mer_timer.c
#	$(GCC) $(CFLAGS) -fomit-frame-pointer -c -o mer_timer.o mer_timer.c
	$(GCC) $(CFLAGS)  -c -o mer_timer.o mer_timer.c
mer_floppy.o : mer_floppy.c mer_floppy.h
	$(GCC) $(CFLAGS) -c -o mer_floppy.o mer_floppy.c

mer_boot: mer_boot.asm
	rm -f mer_boot
	nasm -f bin mer_boot.asm
mer_stup: mer_stup.asm
	rm -f mer_stup
	nasm -f bin mer_stup.asm
clean :
	rm -f mer_krnl.bin $(OBJS) 

install:
	dd if=$(BINARY)/mer_krnl.bin of=/dev/fd0 seek=3
install_all:
	dd if=mer_boot of=/dev/fd0 seek=0
	dd if=mer_stup of=/dev/fd0 seek=1
	dd if=$(BINARY)/mer_krnl.bin of=/dev/fd0 seek=3
backup: 
	tar cfz $(BACKUP)/$(PROJECT)-$(DATE).tgz *.c *.h *.asm README Makefile RELEASENOTES
