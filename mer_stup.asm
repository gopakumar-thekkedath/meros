[BITS 16]
org 0x1000

MOV AX, 0x0
MOV ES, AX
MOV DS, AX
; set up the stack
MOV AX, 0x1000
MOV SS, AX
MOV SP, AX

MOV AX, 0x07c0
MOV DS, AX

call clear_bootcode
call mem_size

; clear the screen
MOV AX, 0x0003
INT 0x10

; Find if we have a HD0 

  MOV AX, 0x1500 ; AH=0x15 ie Get Disk Type
  MOV DL, 0x80   ; DL=0x80 Hard Disk 0
  INT 0x13
  ; Carry Flag set indicates no HD0
  JC no_hd0
  ; If we are here then there is HD0, display a message
  ; and copy the DPT to 7c08-7c18-1
  
  
; Write code to copy HD0 DPT to 0x7c08 - 0x7c18-1
  ; HD0's DPT address is present at IVT's 0x41 entry
  ; Load DS and SI with that address
;  XOR AX, AX
 ; MOV DS, AX
 ; LDS SI, [0x41*4] ; Now DS and SI would have got loaded
 ; MOV AX, 0x07c0
 ; MOV ES, AX ; ES eq 0x07c0
 ; MOV DI, 0x8; ES:DI = 0x7c08
 ; MOV CX, 0x10 ; 16 bytes to mov
 ; CLD
 ; REP
 ; MOVSB

  ;** Make the byte at 0x7c06 '1' to indicate this later to protected code
   ;MOV  [ES:0x29], BYTE 1
  ;JMP chk_hd1

  no_hd0:
 
   chk_hd1:

  ; Find if we have a HD1 
   ;MOV AX, 0x1500 ; AH=0x15 ie Get Disk Type
   ;MOV DL, 0x81   ; DL=0x81 Hard Disk 1
   ;INT 0x13
   ; Carry Flag set indicates no HD0
   ;JC no_hd1
    
; Write code to copy HD1 DPT to 0x7c18 - 0x7c28-1
    ; HD1's DPT is at IVT's 0x46th entry

    ; XOR AX, AX
    ; MOV DS, AX
    ; LDS SI, [0x46*4] ; Now DS and SI would have got loaded
    ; MOV AX, 0x07c0
    ; MOV ES, AX ; ES eq 0x07c0
    ; MOV DI, 0x18; ES:DI = 0x7c18
    ; MOV CX, 0x10 ; 16 bytes to mov
    ; CLD
    ; REP
    ; MOVSB
  
    ; Make the byte at 0x7c07 '1' to indicate this later to protected code
   ;MOV BYTE [ES:0x2A], 1

    ; JMP find_mouse

  ;no_hd1:
  
  ; INT 0x11 will return the System Equipment Data Area information
  ; in register AX, bit '2' will indicate the presance/absence of mouse

  find_mouse:
      XOR AL,AL
      INT 0x11
      TEST AL, 0x4
      JZ enable_A20 ; No pointing device present
      ; Mouse present, store the characted 0x5a at 0x7c28 to indicate this
      MOV [ES:0x28], BYTE 0x5a ; hopefully ES still holds 0x07c0
  enable_A20:

  ; Now we need to enable gate A20 line using keyboard controllers
  ; IO ports, time being skipping the portion where we make sure
  ; that the command queue is empty, hope this works.

   CALL empty_8042   ; This is a straight copy from Linux setup.S
   CALL a20_test
   JNZ a20_done

   MOV AL, 0xD1
   OUT 0x64, AL
   CALL empty_8042   
   	
   MOV AL, 0xDF
   OUT 0x60, AL
   CALL empty_8042

   ; wait until A20 is really enabled, this can take quite some time

a20_kbc_wait:
   XOR CX, CX

a20_kbc_wait_loop:
   CALL a20_test
   JNZ a20_done
   loop a20_kbc_wait_loop    

a20_done:
	
	; copy the memory size from
	; 0x7c00 to 0x1000

	MOV AX, 0x07c0
	MOV DS, AX
	MOV SI, 0x0 ; DS:SI 0x7c00

	MOV AX, 0x0100
	MOV ES, AX
	MOV DI, 0x0; ES:DI 0x1000

	MOV CX, 4
	CLD
	REP 
	MOVSB
	
	;Copy 32 bit protected mode code to 0x5000
        ;//////////////////////

        MOV AX, 0x0500
        MOV ES, AX   ; ES = 0x100
        SUB BX, BX   ; BX = 0, ES:BX = 0x5000

        MOV AX, 0x020f ; AH = 2 ie read sector, AL =15 ie num of sectors
        MOV CX, 0x0004; CH = 0 ie track 0, CL =4 ie sector 4
        MOV DX, 0x0000; DH =0 ie Head 0, DL = 0x00 ie floppy

        INT 0x13 ; 
        ;JC error ; Need to modify here, attemp 3 times atleast !

	;//// 27/08/04 sectors 19-36 ////////
	
	MOV AX, 0x06E0
        MOV ES, AX   ; ES = 0x06E0
        SUB BX, BX   ; BX = 0, ES:BX = 0x6E00

        MOV AX, 0x0212 ; AH = 2 ie read sector, AL =18 ie num of sectors
        MOV CX, 0x0001; CH = 0 ie track 0, CL =01 ie sector 1
        MOV DX, 0x0100; DH =1 ie Head 1, DL = 0x00 ie floppy
        INT 0x13 ; 
        ;JC error ; Need to modify here, attemp 3 times atleast !
        ;////////////////////// sectors 37-54 ///////////
	
	 MOV AX, 0x0920
         MOV ES, AX   ; ES = 0x0920
         SUB BX, BX   ; BX = 0, ES:BX = 0x9200

         MOV AX, 0x0212 ; AH = 2 ie read sector, AL =18 ie num of sectors
         MOV CX, 0x0101; CH = 1 ie track 1, CL =01 ie sector 1
         MOV DX, 0x0000; DH =0 ie Head 0, DL = 0x00 ie floppy
         INT 0x13 ; 
        ;JC error ; Need to modify here, attemp 3 times atleast !
        ;////////////////////// sectors 55-72 ///////////
	
	 MOV AX, 0x0b60
         MOV ES, AX   ; ES = 0xb60
         SUB BX, BX   ; BX = 0, ES:BX = 0xb600

         MOV AX, 0x0212 ; AH = 2 ie read sector, AL =18 ie num of sectors
         MOV CX, 0x0101; CH = 1 ie track 1, CL =01 ie sector 1
         MOV DX, 0x0100; DH =1 ie Head 1, DL = 0x00 ie floppy
         INT 0x13 ; 
         ;JC error ; Need to modify here, attemp 3 times atleast !
 
; ** Now it is time to enter the wonderful world of protected mode !!**
        ; The steps required to enter protected mode are
        ; a) Create a valid GDT
        ; b) Create a valid IDT
        ; c) Disable interrupts  (NMI also)
        ; d) Point GDTR to GDT
        ; e) Point IDTR to IDT
        ; f) Re program the PIC
        ; g) Set the PE bit in MSW register
        ; h) Do a FAR JMP (load both CS and EIP) to enter protected mode
        ; i) Load DS and SS with data/stack segment selector
        ; j) Setup the protected mode stack
        ; k) re-enable interrupts ( and get screwed !!)
        
	; Disable Maskable Interrupts
         CLI
        ; Disable NMI
         MOV AL, 0x80
         OUT 0x70, AL ; Setting bit 7 of port 0x70 disables NMI

	XOR AX,AX
	MOV DS, AX

; Load IDTR
  	LIDT  [IDT_48]

;Load GDTR
	LGDT [gdt_desc]
        
	; Now we need to re program the 8259 PIC
        ;
        ;  Reprogramming the PIC is required to enter protected mode
        ;  as in protected mode interrupt vectors 0-31 are dedicated
        ;  for s/w exceptions.
        ;  PIC Master 0x20-0x21h, Slave 0xA0 and 0xA1
        ;
          ;ICW1 ,
          MOV AL, 0x11 ; 0x11 is 10001b, ICW4 is sent, cascaded PICs, edge
                       ; triggering.
          OUT 0x20, AL ; sending ICW1 to master
          NOP
          NOP          ; small delay
          OUT 0xA0, AL ; Sending ICW1 to slave

          ; The ICW1 must be followed by ICW2, ICW2 indicates the address
          ; of IRQ0 in the vector table. This is what we need to change
          ; when in protected mode, we want IRQ 0 = vector 32 and so on 
          ; Only the 5 most significant bits of the vector number are used.

          ;Master's IRQ0 = vector 32
          MOV AL, 0x20
          OUT 0x21, AL
          NOP
          NOP
          ;Slave's IRQ0 = vector 40
          MOV AL, 0x28
          OUT 0xA1, AL
          NOP
          NOP
          ;ICW3 needs to be send only if there are cascaded PICs
          ;as we have it in our machine, this in necessary for us
          ; The command is given through 0x21 and 0xA1
          ;In case of master, each bit corresponds to an IRQ line, 
	  ;if cleared then
          ;it is connected to a peripheral, if set it set then, it is connected
          ;to a slave PIC.
          ;In case of slave, last 3 bits gives the IRQ number of master
          ;to which it is connected

          MOV AL, 0x4
          OUT 0x21, AL ; Master, slave is connected to IRQ2
          NOP
          NOP
          MOV AL, 0x2
          OUT 0xA1, AL ; Slave, connected to IRQ2 of master.
          NOP
          NOP
          ; The final command is ICW4, Bit 0 should be set for PC's
          ; as it indicates 80x86 architecture
          ; Bit 1 if set to '1' implies s/w generated EOI
          MOV AL, 0x1
          OUT 0x21, AL
          NOP
          NOP
          OUT 0xA1, AL

	

 
	; Enable protected mode by setting the PE bit in CR0
	MOV EAX, CR0
	OR EAX, 1
	MOV CR0, EAX

	  ; Flush the instruction prefetch queue as we are going to enter
	  ; 32 bit world
	  JMP flush_all

flush_all:


 JMP 0x10:0x5000; jump to where the 32 bit protected mode code resides.

; We should not be here !!!!
hang:
jmp hang

error:
	reboot DB 0xea
	       DW 0x0000
	       DW 0xffff

[BITS 16]
;Function to clear the memory area that holds the boot code
clear_bootcode:
	;DS register holds 0x07c0
	SUB SI, SI ; DS:SI = 0x7c00
	MOV CX, 500
   cont_clear:
	MOV BYTE [DS:SI], 0x0
	DEC CX
	JZ done_clear
	INC SI
	JMP cont_clear
   done_clear
	ret	


mem_size:
	; calculate the total physical mem size in KB and store it
	; at 0x7c00 -0x7c03
	
        MOV AX, 0xE801
        INT 0x15
        JC err_mem

        ; EBX will hold the number of 64 K blocks and this needs to
        ; be shifted 6 times left to get value in KBytes

        shl EBX, 6
        ; save the value in address 0x7c00-0x7c03
        MOV [DS:0x0], EBX
        ; add contents of EAX to it
        ADD [DS:0x0], EAX
      err_mem:
        ret ; return back 
  
empty_8042:
	PUSH ECX
	MOV ECX, 100000
empty_8042_loop:
	DEC ECX
	JZ empty_8042_end_loop
	CALL delay
        IN AL, 0x64
 	TEST AL, 0x1
	JZ no_output
	
	CALL delay
	IN AL, 0x60
	JMP empty_8042_loop

no_output:
	TEST AL, 0x2
	JNZ empty_8042_loop	
empty_8042_end_loop:
	POP ECX
	RET

a20_test:
	PUSH CX
	PUSH AX
	XOR CX, CX
	MOV FS, CX ; FS=0x0
	DEC CX
	MOV GS, CX ; GS=0xFFFF
	MOV CX, 20
	MOV AX, [FS: 0x4*0x80]
	PUSH AX
	
a20_test_wait:
	INC AX
	MOV [FS:0x4*0x80], AX
	call delay
	CMP AX, [GS: 0x4*0x80+0x10]
	LOOPE a20_test_wait

	POP WORD [FS: 0x4*0x80]
	POP AX
	POP CX
	ret

delay:
	OUT 0x80, AL
	ret
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

gdt_end:

gdt_desc:
dw gdt_end - gdt -1
;dd gdt + 0x7e00
;dd gdt + 0x8e00
dd gdt  

IDT_48:
	dw 0x0 ; IDT Limit, for the time being zero
	dw 0x0;  IDT Base, again, zero for the time being.
 	dw 0x0;

  mem_msg DB 'Physical Memory Size :', 0
  hd0_yes DB '*HD0 Present',0
  hd0_no  DB '*No HD0',0
  hd1_yes DB '*HD1 Present',0
  hd1_no  DB '*No HD1',0
  mouse_yes DB '*Mouse Detected',0
;times 512-($-$$) db 0
times 1024-($-$$) db 0

