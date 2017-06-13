;
; MEROS boot loader, loads setup code available in floppy's
; first sector to 0x7e00 
; 
; 4/4/04, modified code to read two sectors of setup code
; to memory at 0x7e00

_main:
        ; Set Video in text mode

        MOV AX, 0x0003
        INT 0x10

        ; ES should be zero, just make sure

        MOV AX, 0x0
        MOV ES, AX


        ; Copy setup code to 0x7e00

       ; MOV AX, 0x07e0
       ;MOV AX, 0x08e0
	;MOV AX, 0x7e0
	MOV AX, 0x100
        MOV ES, AX   ; ES = 0x100
        SUB BX, BX   ; BX = 0, ES:BX = 0x1000

        ;MOV AX, 0x0201 ; AH = 2 ie read sector, AL =1 ie num of sectors
        MOV AX, 0x0202 ; AH = 2 ie read sector, AL =2 ie num of sectors
        MOV CX, 0x0002; CH = 0 ie track 0, CL =2 ie sector 2
        MOV DX, 0x0000; DH =0 ie Head 0, DL = 0x00 ie floppy

        INT 0x13 ; 
        JC error ; Need to modify here, attemp 3 times atleast !


       ; JMP 0x07e0:0000
       ; JMP 0x08e0:0000
       
	; JMP 0x7e0:0000
	 JMP 0x100:0000
        ; We should not be here unless there is an error
 error:
        MOV AX, 0x00
        MOV ES, AX ; ES = 0x00

        MOV AX, 0x1301 ; Display the string
        MOV BX, 0x0007
        MOV CX, 0x15
        MOV BP, err_msg
        ADD BP, 0x7c00
        INT 0x10
                 
        ; Wait for the key stroke
        MOV AH, 0x00
        INT 0x16


        reboot DB 0xEA
                DW 0x0000
                DW 0xFFFF

  msg DB 'Loading MEROS .....',0
  err_msg DB 'WE SHOULD NOT BE HERE',0


        times 510-($-$$) db 0
        sig DW 0xAA55;      Boot signature
