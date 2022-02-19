; ====================================================================
; Project:   DumpMaster64 - Fast IEC Implementation for 1541 - Upload
; Version:   v1.3
; Year:      2022
; Author:    Stefan Wagner
; Github:    https://github.com/wagiminator
; EasyEDA:   https://easyeda.com/wagiminator
; License:   http://creativecommons.org/licenses/by-sa/3.0/
; ====================================================================
;
; Description:
; ------------
; Simple fast data transfer by accelerating the IEC communication due
; to the use of an asynchronous 2-bit parallel protocol. This program
; is loaded into the memory of the floppy disk drive and communicates
; from there with the DumpMaster64 adapter.
; This implementation writes a number of bytes to the specified RAM
; address.
;
; References:
; -----------
; Michael Steil: https://www.pagetable.com/?p=568
;
; Assembling Instructions:
; ------------------------
; ca65 -l -t c64 fastupload.a65
; ld65 -t c64 -o fastupload.bin fastupload.o
;
; Operating Instructions:
; -----------------------
; "M-E"<addrLow><addrHigh><RAMlow><RAMhigh><size>
;
; $0200 - $0202 "M-E"       Memory Execute command
; $0203 - $0204 <addrL/H>   start address of this program in RAM ($0400)
; $0205 - $0206 <RAMl/h>    RAM address to start writing to
; $0207         <size>      number of bytes to be written (0=256 bytes)


.setcpu "6502"
.org $0400

; Initial setup
; -------------
start:
    lda $0205         ; low byte RAM address
    sta $14
    lda $0206         ; high byte RAM address
    sta $15
    ldx $0207         ; bytes counter
    ldy #$00          ; RAM pointer

; Wait for adapter to be ready
; ----------------------------
    lda #$01          ; wait for adapter 'READY TO SEND BLOCK':
waitready:
    bit $1800         ; test DATA line
    beq waitready     ; wait for DATA LOW

; Receive data bytes via fast IEC and write them into RAM area
; ------------------------------------------------------------
    sei               ; timed sequence coming up
receivebyte:
    lda #$08          ; 2 declare 'READY TO RECEIVE BYTE':
    sta $1800         ; 4 -> pull CLK LOW
    lsr               ; 2 declare 'LETS GO':
    sta $1800         ; 4 -> release CLK HIGH
    lda $1800         ; 4 get bits 3 and 1
    asl               ; 2
    ora $1800         ; 4 get bits 2 and 0
    and #$0F          ; 2
    sta $10           ; 3 store low nibble
    lda $1800         ; 4 get bits 7 and 5
    asl               ; 2
    ora $1800         ; 4 get bits 6 and 4
    asl               ; 2
    asl               ; 2
    asl               ; 2
    asl               ; 2 high nibble
    ora $10           ; 3 -> combine with low nibble
    sta ($14),y       ; 6 write byte into RAM
    iny               ; 2 increase RAM pointer
    dex               ; 2 decrease bytes counter
    bne receivebyte   ; 3 repeat for all bytes
    cli               ; interrupts are allowed again
    rts               ; mission accomplished
