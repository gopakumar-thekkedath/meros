;
; 32 bit protected mode code, loaded at 0x1000 (physical address)
;
[BITS 32]
; set all data segment and stack segment selector value to the
;4th entry of GDT
EXTERN mer_start
EXTERN keyboard_tasklet
EXTERN do_irq
EXTERN schedule
EXTERN dont_schedule

global meros_pg_dir
global meros_pg0
global test_value
global flush_tlb
global meros_IDT
global read_port_char
global write_port_char
global enable_kbd
global keybrd_isr
global clear_intr
global set_intr
global isr_IRQ0
global isr_IRQ1
global isr_IRQ2
global isr_IRQ3
global isr_IRQ4
global isr_IRQ5
global isr_IRQ6
global isr_IRQ7
global isr_IRQ8
global isr_IRQ9
global isr_IRQ10
global isr_IRQ11
global isr_IRQ12
global isr_IRQ13
global isr_IRQ14
global isr_IRQ15
global gdt_desc
global restore_all
global trial

MOV AX, 0x18
MOV DS, AX
MOV ES, AX
MOV FS, AX
MOV GS, AX
MOV SS, AX
MOV ESP, 0x90000 ; Let the 32 bit stack begins from 0x90000

MOV BYTE [0xB8000], 'G'
MOV BYTE [0xB8001], 1Bh
MOV BYTE [0xB8002], 'o'
MOV BYTE [0xB8003], 1Bh

; 03/07/2004, load the GDTR again as we are going to have a new GDT
; We subtract gdt_desc by 0xc0000000 because of the following reason,
; we are getting loaded at physical address 0x1000.
; We are linked at 0xc0001000, so if we were getting loaded at physical
; addres 0x0 then we should subtract the symbols with 0xc0001000 , but
; as we are loaded at 0x1000, we should subtract with (0xc0001000 - 0x1000)
; ie 0xc0000000

LGDT [gdt_desc -0xc0000000]

; Time to set up the page tables
; We will have for the time being, mapping for the first 4 MB
; only, both from 0x0-0x4-1 MB and 0xc000000 to 0xc000004-1 MB

; First set up the PGD to map 4MB of memory
MOV EAX, meros_pg0 - 0xc0000000
;ADD EAX, 0x007; Present, Read, Write permission for the 1st entry 
ADD EAX, 0x001; Only Present, Accessible only from PL 0, 1 or 2 
MOV [meros_pg_dir -0xc0000000], EAX  ; set the 1st entry
MOV [meros_pg_dir + 768*4 -0xc0000000], EAX  ; set the 768th entry

; Now set up the first page table to map the 1st 4MB of physical memory

MOV EDI, meros_pg0  -0xc0000000; ES:EDI points to PG0
SUB EAX, EAX
;ADD EAX, 0x007
ADD EAX, 0x001 ; modified on 25/06/04 so that only PL 0,1 or 2 can access

cont:
STOSD ; content of EAX is stored at ES:EDI and EDI gets incremented by 4
ADD EAX, 0x1000 ; Next page, Next page and so on
MOV EBX, meros_pgend - 0xc0000000
CMP EBX, EDI
JNZ cont

; Enable paging, 
; first load the meros_pg_dir address to CR3
; Then set the PG flag(bit 31 of CR0)

MOV EAX, meros_pg_dir - 0xc0000000
MOV CR3, EAX ; CR3 loaded with physical address of PGD
MOV EAX, CR0
OR EAX, 0x80000000
MOV CR0, EAX ; This should enable paging, hope it won't screw us !!!
JMP flush_all 
flush_all:

 ; Now try to write using 0xc0000000

MOV BYTE [0xc0000000 + 0xB8008], 'g'
MOV BYTE [0xc0000000 + 0xB8009], 1Bh
MOV BYTE [0xc0000000 + 0xB800A], 'o'
MOV BYTE [0xc0000000 + 0xB800B], 1Bh;

; Time for IDT set up, we will use EAX and EBX for holding descriptors

CLI

MOV EBX, spurious_int 
MOV EAX, 0x10 ; code  segment selector
SHL EAX, 16   
MOV AX, BX   ; LSB 16 bits of spurios_int address in EAX
MOV BX, 0xEE00 ; DPL=3, P

;fill the 256 entries of IDT with EBX:EAX
MOV CX, 256
MOV EDI, meros_IDT 
cont_IDT:
MOV [DI], EAX
MOV [DI + 4], EBX
ADD DI, 8
DEC CX
JNZ cont_IDT

; Reload IDTR

LIDT [meros_IDT_desc ]
; Now, enable interrupts and get screwed

;STI

MOV BYTE [0xB800C],'9'
MOV BYTE [0xB800D],1Eh


;//// call the display function, for displaying mem size ////
;PUSH DWORD mem_msg 
;PUSH DWORD 23
;PUSH DWORD 'c'
;CALL mprint
;ADD ESP, 12



;PUSH DWORD 0x7c00 
;PUSH DWORD 5
;PUSH DWORD 'd'
;CALL mprint
;ADD ESP, 12
;//// Display the presence/absence of hdisks/////
; If HD0 is present then [0x7c29] =1, if HD1 then [0x7c2A]=1

;	XOR EAX, EAX
;	MOV WORD AX, [0x7c29]
;	CMP AL, 0x1
;	JNE chk_disk1
	; Display the presence of HD0
;	PUSH DWORD hd0_yes 
;	PUSH DWORD 11
;	PUSH DWORD 'c'
;	CALL mprint
;	ADD ESP, 12
	;
	;
;chk_disk1:
;	CMP AH, 0x1
;	JNE chk_mouse 
	; Display the presence of HD1	
;	PUSH DWORD hd1_yes 
;	PUSH DWORD 11
;	PUSH DWORD 'c'
;	CALL mprint
;	ADD ESP, 12
	;
;chk_mouse:
;	MOV AL, BYTE [0x7c28]
;	CMP AL, 0x5a
;	JNE hang
	; Display the presence of Mouse	
;	PUSH DWORD mouse_yes 
;	PUSH DWORD 14
;	PUSH DWORD 'c'
;	CALL mprint
;	ADD ESP, 12
;HANG_ON:
;	JMP HANG_ON
call mer_start

hang:
JMP hang

keybrd_isr:
	; Read the byte from kbd port 0x60
;	IN AL, 0x60
;	MOV BYTE [0xB820C], AL
;	MOV BYTE [0xB820D],1Eh
;	call keyboard_tasklet
	MOV AL, 0x20 ; send EOI to master
	OUT 0x20, AL
	IRET 

spurious_int:
	MOV BYTE [0xB820A],'I'
	MOV BYTE [0xB820B],1Eh
	MOV BYTE [0xB820C],'N'
	MOV BYTE [0xB820D],1Eh
	MOV AL, 0x20 ; send EOI to master
	OUT 0x20, AL
HANG_ON:
	JMP HANG_ON
	iret

flush_tlb:
	MOV EAX, CR3
	MOV CR3, EAX

read_port_char:
	PUSH EBP
	MOV EBP, ESP
	; Need to read a byte from port EBP+8,
	XOR EAX, EAX
	PUSH  WORD DX
	MOV DX, WORD [EBP + 8]
	IN AL, DX  ; [EBP + 8] 
;	MOV EAX, [EBP + 8]
	POP  WORD DX
	POP EBP
	RET 

write_port_char:
	PUSH EBP
	MOV EBP, ESP
	; write the value at [EBP + 12] to port num [EBP + 8]
	XOR EAX, EAX
	MOV EAX, [EBP + 12] ; value in to EAX
	PUSH WORD DX
	MOV DX, [EBP + 8]
	OUT DX, AL
	POP WORD DX
	POP EBP
	RET

enable_kbd:
 start:
	PUSH EAX
	IN AL, 0x64
	TEST AL, 0x2
	JNZ start
	MOV AL, 0xae
	OUT 0x64, AL
	POP EAX
	RET

clear_intr:
	CLI
	RET

set_intr:
	STI
	RET


;////// ISR ////////////
isr_IRQ0:

	push word es
	push word ds
	push dword eax
	push dword ecx
	push dword ebp
	push dword edi
	push dword esi
	push dword edx
	push dword ecx
	push dword ebx
	push dword  0
	call do_irq	;get rid of EIP and 0
	
; store the current ESP in task_struct->ESP ?? may be we can have ECX
; register holding the current task's task_struct and EBX holding new
; task's task_struct address. Note: both this could point to the same
; task struct if there is no need for a task switch
; EBX register is suppose to hold new task_struct address so load ESP with
; EBX->ESP ie MOV ESP, [EBX + 0] as the first field of task_struct is ESP
;
;	17/08/2004
;	change in plan, here we only check if current 
;	task_struct->need_resched = 1, if so then we call a function
; 	called schedule() to perform a process switch. else we proceed ..
;	Note: when we are here, we expect EBX to hold the starting address
;	of a process's task_struct.
;

;	mov ebx, esp	; move the current stack pointer to EBX register
;	and ebx, 0xfffff000 ; get the base of current task_struct
;	cmp DWORD [ebx + 4], 1 ; check if need_sched = 1, if so call schedule
;	jne no_schedule
;	mov DWORD [ebx +4], 0  ; set the need_sched back to zero
;	call schedule		
;	jmp restore_all

;no_schedule:
;	call dont_schedule
;restore_all:
	add esp ,4	
	pop dword ebx
	pop dword ecx
	pop dword edx
	pop dword esi
	pop dword edi
	pop dword ebp
	pop dword ecx
	pop dword eax
	pop word ds
	pop word es
	mov al, 0x20 ; send EOI to master
	out 0x20, al
	iret
trial:
	add esp ,4
	pop dword ebx
	pop dword ecx
	pop dword edx
	pop dword esi
	pop dword edi
	pop dword ebp
	pop dword ecx
	pop dword eax
	pop word ds
	pop word es
	mov al, 0x20 ; send EOI to master
	out 0x20, al
	iret
isr_IRQ1:
	push word es
	push word ds
	push dword eax
	push dword ecx
	push dword ebp
	push dword edi
	push dword esi
	push dword edx
	push dword ecx
	push dword ebx
	push dword 1 
	call do_irq	
	add esp ,4	
	pop dword ebx
	pop dword ecx
	pop dword edx
	pop dword esi
	pop dword edi
	pop dword ebp
	pop dword ecx
	pop dword eax
	pop word ds
	pop word es
	mov al, 0x20 ; send EOI to master
	out 0x20, al
	iret

isr_IRQ2:
	push word es
	push word ds
	push dword eax
	push dword ecx
	push dword ebp
	push dword edi
	push dword esi
	push dword edx
	push dword ecx
	push dword ebx
	push dword 2 
	call do_irq	
	add esp ,4	
	pop dword ebx
	pop dword ecx
	pop dword edx
	pop dword esi
	pop dword edi
	pop dword ebp
	pop dword ecx
	pop dword eax
	pop word ds
	pop word es
	mov al, 0x20 ; send EOI to master
	out 0x20, al
	iret
isr_IRQ3:
	push word es
	push word ds
	push dword eax
	push dword ecx
	push dword ebp
	push dword edi
	push dword esi
	push dword edx
	push dword ecx
	push dword ebx
	push dword 3 
	call do_irq	
	add esp ,4	
	pop dword ebx
	pop dword ecx
	pop dword edx
	pop dword esi
	pop dword edi
	pop dword ebp
	pop dword ecx
	pop dword eax
	pop word ds
	pop word es
	mov al, 0x20 ; send EOI to master
	out 0x20, al
	iret
isr_IRQ4:
	push word es
	push word ds
	push dword eax
	push dword ecx
	push dword ebp
	push dword edi
	push dword esi
	push dword edx
	push dword ecx
	push dword ebx
	push dword 4 
	call do_irq	
	add esp ,4	
	pop dword ebx
	pop dword ecx
	pop dword edx
	pop dword esi
	pop dword edi
	pop dword ebp
	pop dword ecx
	pop dword eax
	pop word ds
	pop word es
	mov al, 0x20 ; send EOI to master
	out 0x20, al
	iret
isr_IRQ5:
	push word es
	push word ds
	push dword eax
	push dword ecx
	push dword ebp
	push dword edi
	push dword esi
	push dword edx
	push dword ecx
	push dword ebx
	push dword 5 
	call do_irq	
	add esp ,4	
	pop dword ebx
	pop dword ecx
	pop dword edx
	pop dword esi
	pop dword edi
	pop dword ebp
	pop dword ecx
	pop dword eax
	pop word ds
	pop word es
	mov al, 0x20 ; send EOI to master
	out 0x20, al
	iret
isr_IRQ6:
	push word es
	push word ds
	push dword eax
	push dword ecx
	push dword ebp
	push dword edi
	push dword esi
	push dword edx
	push dword ecx
	push dword ebx
	push dword 6 
	call do_irq	
	add esp ,4	
	pop dword ebx
	pop dword ecx
	pop dword edx
	pop dword esi
	pop dword edi
	pop dword ebp
	pop dword ecx
	pop dword eax
	pop word ds
	pop word es
	mov al, 0x20 ; send EOI to master
	out 0x20, al
	iret

isr_IRQ7:
	push word es
	push word ds
	push dword eax
	push dword ecx
	push dword ebp
	push dword edi
	push dword esi
	push dword edx
	push dword ecx
	push dword ebx
	push dword 7 
	call do_irq	
	add esp ,4	
	pop dword ebx
	pop dword ecx
	pop dword edx
	pop dword esi
	pop dword edi
	pop dword ebp
	pop dword ecx
	pop dword eax
	pop word ds
	pop word es
	mov al, 0x20 ; send EOI to master
	out 0x20, al
	iret

isr_IRQ8:
	push word es
	push word ds
	push dword eax
	push dword ecx
	push dword ebp
	push dword edi
	push dword esi
	push dword edx
	push dword ecx
	push dword ebx
	push dword 8 
	call do_irq	
	add esp ,4	
	pop dword ebx
	pop dword ecx
	pop dword edx
	pop dword esi
	pop dword edi
	pop dword ebp
	pop dword ecx
	pop dword eax
	pop word ds
	pop word es
	mov al, 0x20 ; send EOI to master
	out 0x20, al
	iret
isr_IRQ9:
	push word es
	push word ds
	push dword eax
	push dword ecx
	push dword ebp
	push dword edi
	push dword esi
	push dword edx
	push dword ecx
	push dword ebx
	push dword 9 
	call do_irq	
	add esp ,4	
	pop dword ebx
	pop dword ecx
	pop dword edx
	pop dword esi
	pop dword edi
	pop dword ebp
	pop dword ecx
	pop dword eax
	pop word ds
	pop word es
	mov al, 0x20 ; send EOI to master
	out 0x20, al
	iret

isr_IRQ10:
	push word es
	push word ds
	push dword eax
	push dword ecx
	push dword ebp
	push dword edi
	push dword esi
	push dword edx
	push dword ecx
	push dword ebx
	push dword 10 
	call do_irq	
	add esp ,4	
	pop dword ebx
	pop dword ecx
	pop dword edx
	pop dword esi
	pop dword edi
	pop dword ebp
	pop dword ecx
	pop dword eax
	pop word ds
	pop word es
	mov al, 0x20 ; send EOI to master
	out 0x20, al
	iret

isr_IRQ11:
	push word es
	push word ds
	push dword eax
	push dword ecx
	push dword ebp
	push dword edi
	push dword esi
	push dword edx
	push dword ecx
	push dword ebx
	push dword 11
	call do_irq	
;	call keyboard_tasklet
	add esp ,4	
	pop dword ebx
	pop dword ecx
	pop dword edx
	pop dword esi
	pop dword edi
	pop dword ebp
	pop dword ecx
	pop dword eax
	pop word ds
	pop word es
	mov al, 0x20 ; send EOI to master
	out 0x20, al
	iret
isr_IRQ12:
	push word es
	push word ds
	push dword eax
	push dword ecx
	push dword ebp
	push dword edi
	push dword esi
	push dword edx
	push dword ecx
	push dword ebx
	push dword 12 
	call do_irq	
	add esp ,4	
	pop dword ebx
	pop dword ecx
	pop dword edx
	pop dword esi
	pop dword edi
	pop dword ebp
	pop dword ecx
	pop dword eax
	pop word ds
	pop word es
	mov al, 0x20 ; send EOI to master
	out 0x20, al
	iret
isr_IRQ13:
	push word es
	push word ds
	push dword eax
	push dword ecx
	push dword ebp
	push dword edi
	push dword esi
	push dword edx
	push dword ecx
	push dword ebx
	push dword 13 
	call do_irq	
	add esp ,4	
	pop dword ebx
	pop dword ecx
	pop dword edx
	pop dword esi
	pop dword edi
	pop dword ebp
	pop dword ecx
	pop dword eax
	pop word ds
	pop word es
	mov al, 0x20 ; send EOI to master
	out 0x20, al
	iret

isr_IRQ14:
	push word es
	push word ds
	push dword eax
	push dword ecx
	push dword ebp
	push dword edi
	push dword esi
	push dword edx
	push dword ecx
	push dword ebx
	push dword 14 
	call do_irq	
	add esp ,4	
	pop dword ebx
	pop dword ecx
	pop dword edx
	pop dword esi
	pop dword edi
	pop dword ebp
	pop dword ecx
	pop dword eax
	pop word ds
	pop word es
	mov al, 0x20 ; send EOI to master
	out 0x20, al
	iret

isr_IRQ15:
	push word es
	push word ds
	push dword eax
	push dword ecx
	push dword ebp
	push dword edi
	push dword esi
	push dword edx
	push dword ecx
	push dword ebx
	push dword 15 
	call do_irq	
	add esp ,4	
	pop dword ebx
	pop dword ecx
	pop dword edx
	pop dword esi
	pop dword edi
	pop dword ebp
	pop dword ecx
	pop dword eax
	pop word ds
	pop word es
	mov al, 0x20 ; send EOI to master
	out 0x20, al
	iret

;//////////////////

; Function to display the contents of the memory location
  mprint:
        PUSH  EBP
        MOV EBP, ESP
        MOV EAX, DWORD [EBP+16] ; offset addres ie 4(EBP)+  4(EIP) + 4(char|dec) 				;+ 4(len)
        MOV ESI, EAX; ESI is the source offset
        MOV ECX, DWORD [EBP+12] ; CX eq number of bytes to display

        ; Now ESI points to the memory location containing string
        ; ECX contains numbr of bytes to print

        MOV EAX, 0xB8000
        MOV EDI, EAX ; EDI eq 0xB8000

       ; Read the cursor from 0x7c04-0x7c05 and store it at EDI
	XOR EAX, EAX
	MOV AX, WORD [0x7c04]
	ADD EDI, EAX
        
; If the first character of the string is a newline then adjust
        ; cursor accordingly
        MOV AH, BYTE [ESI] ; byte from source address to AH
        CMP AH, '*'
        JNE store_cursor; Not a newline
  
      ; If here we should display this string in a new line
  ;      MOV EDX, 0 ; As it should hold the reminder
  ;      MOV EAX, EDI ; AX now holds current cursor location
  ;      MOV EBX, 160 ; 
  ;      DIV EBX ; Now DX should hold the reminder
  ;      SUB EBX, EDX ; BX contains the value that should be added to current
                         ; cursor location
  ;      ADD EDI, EBX
  ; update cursor location content too
  ; ADD WORD [0x7c04], BX
	;///// NEW METHOD
;	MOV EAX, EDI ; EAX holds current cursor position
	;TODO Commented above line
cont_with:
	CMP EAX, 160
	JL done_with
	SUB EAX, 160
	JMP cont_with
done_with:
	MOV EBX, EAX ; 
	MOV EAX, 160
	SUB EAX, EBX ; EAX holds the value to be added to current cursor
	ADD EDI, EAX
	ADD WORD [0x7c04], AX
	;/////////////
        ;Move the source pointer by '1' byte to get rid of new line symbol
        ADD ESI, 1

     store_cursor:
        ; Store the new cursor value back to 0x7c04 & 5
        MOV EAX, ECX
      ;  MOV EDX, 2
      ;  MUL EDX ; AX = AX *DH ,Now AX should hold value to be added to [0x7c04]
	ADD EAX, ECX ; trial
        ADD WORD [0x7c04], AX

       ;EDI gives the starting of video memory

        ;Find if the display material is string or decimal

        CMP BYTE [EBP+8], 'c'
        JNE print_dec
                       
       ; If we are here, it is a string that we need to print character
   continue:
        MOV AH, BYTE [ESI] ; byte from source address to AH
        MOV [EDI], AH; write to video buffer
	MOV BYTE [EDI+1],  0x1E
        DEC ECX
        JZ done
        ADD EDI, 2
        ADD ESI, 1
        JMP continue

      print_dec:
        ; If here, then need to print a decimal

        ; CX is going to be used to count the number of bytes to display
        MOV ECX, 0
        ; copy 4 bytes from specified memory location to EAX register
        MOV EAX, DWORD [ESI]
        MOV EBX, 10  ; EDX holds the divisor (10)
               
      cont_div:
        MOV EDX, 0   ; As EDX should hold the reminder
        DIV EBX      ; This will result in EDX holding reminder and EAX quotient
        ADD EDX, 48 ; Add 48 as we want it in ascii

        ;Push the reminder to stack
        PUSH  EDX
        ; count of number of digits
        INC ECX

        CMP EAX, 0
        JE done_div
   
        JMP cont_div

     done_div:
        ; Pop out the digits from stack and display
        MOV EAX,0
        POP  EAX
        MOV [EDI], AL
	MOV BYTE [EDI + 1], 0x1E
        DEC ECX
        CMP ECX, 0
        JE done
        ADD EDI, 2
        JMP done_div
     done:
        POP EBP
	ret
;///////////////////
mem_msg DB 'Physical Memory Size :', 0
hd0_yes DB '*HD0 PRESENT', 0
hd0_no  DB '*No HD0', 0
hd1_yes DB '*HD1 Present', 0
hd1_no  DB '*No HD1', 0
mouse_yes DB '*Mouse Detected', 0
test_value  DD 100
trial_msg DB 'popped ECX value:', 0
;////////////////
;// 03/07/04, new GDT (with user mode code and data segment) //

gdt;

gdt_null:
dd 0
dd 0

gdt_notused:
dd 0
dd 0

gdt_code:
dw 0xFFFF
dw 0x0
dw 0x9A00
dw 0x00CF

gdt_data:
dw 0xFFFF
dw 0x0
dw 0x9200
dw 0x00CF

gdt_code_user:
dw 0xFFFF
dw 0x0
dw 0xFA00
dw 0x00CF

gdt_data_user:
dw 0xFFFF
dw 0x0
dw 0xF200
dw 0x00CF

gdt_tss:
dw 0xFFFF
dw 0x0000
dw 0xE900	
dw 0x00CF

gdt_end:

gdt_desc:
dw gdt_end - gdt -1
dd gdt -0xc0000000 ; as we are linked at 0xc0001000

meros_IDT:
times 256*8 db 0
meros_IDT_desc:
	 DW 256*8
	 DD meros_IDT -0xc0000000
times 0x1000 -($-$$) db 0
meros_pg_dir: ; Page Directory

times 0x2000 -($-$$) db 0
meros_pg0:	; Page Table, maps the first 4 MB of memory

times 0x3000 -($-$$) db 0
meros_pgend




