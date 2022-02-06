; ====================================================================
; Project:   DiskBuddy64 - Fast IEC Implementation for 1541 - Loading
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
; Simple fast loading by accelerating the IEC communication due to
; the use of an asynchronous 2-bit parallel protocol. This program is
; loaded into the memory of the floppy disk drive and communicates
; from there with the DiskBuddy64 adapter.
; This implementation reads a file starting at the specified track
; and sector.
;
; References:
; -----------
; Michael Steil: https://www.pagetable.com/?p=568
;
; Assembling Instructions:
; ------------------------
; ca65 -l -t c64 fastread.a65
; ld65 -t c64 -o fastread.bin fastread.o
;
; Operating Instructions:
; -----------------------
; "M-E"<addrLow><addrHigh><track><sector>
;
; $0200 - $0202 "M-E"       Memory Execute command
; $0203 - $0204 <addrL/H>   start address of this program in RAM ($0500)
; $0205         <track>     track on disk to start reading
; $0206         <sector>    sector on track to start reading


.setcpu "6502"
.org $0500


; Initial setup
; -------------
start:
    lda #$00          ; set buffer number:
    sta $f9           ; -> buffer at $0300
    lda $0205         ; get track from command buffer
    sta $06           ; set track for disk operation
    lda $0206         ; get sector from command buffer
    sta $07           ; set sector for disk operation
    jsr $c118         ; turn on DRIVE LED
    lda #$12          ; speed up stepper
    sta $1c07
    jsr $c63d         ; check drive and initialize
    bne finish        ; 'READ ERROR' -> finish

; Read sector from disk
; ---------------------
readsector:   
    ldx #$05          ; number of retries
retry:
    lda #$80          ; job number for reading sector
    sta $00           ; set job -> start disk operation
waitcomplete:
    lda $00
    bmi waitcomplete  ; wait for job to complete
    cmp #$01          ; was it successful?
    beq sendblock     ; -> send data block via IEC
    dex               ; decrease retry counter
    bne retry         ; try again (max 5x)
    beq finish        ; 'READ ERROR' -> terminate

; Send 256 bytes to adapter via fast IEC
; --------------------------------------
sendblock:
    sei               ; disable interrupts
    ldx #$00          ; buffer index
sendbyte:
    lda $0300,x       ; 4 read byte from buffer
    ldy #$08          ; 2 mark 'START SENDING BYTE':
    sty $1800         ; 4 -> pull CLK LOW
    tay               ; 2 save original in y
    lsr               ; 2 get high nibble
    lsr               ; 2
    lsr               ; 2
    lsr               ; 2
    sta $1800         ; 4 transfer bit 7 and 5
    asl               ; 2
    and #$0F          ; 2
    sta $1800         ; 4 transfer bit 6 and 4
    tya               ; 2 get original byte
    and #$0F          ; 2 get low nibble
    sta $1800         ; 4 transfer bit 3 and 1
    asl               ; 2
    and #$0F          ; 2
    sta $1800         ; 4 transfer bit 2 and 0
    ldy #$00          ; 2 mark 'END OF BYTE':
    sty $1800         ; 4 -> release CLK HIGH
    inx               ; 2
    bne sendbyte      ; 3 repeat for all 256 bytes
    cli               ; enable interrupts

; Prepare next sector
; -------------------
    lda $0301         ; get next sector
    sta $07           ; set sector for disk operation
    lda $0300         ; get next track
    sta $06           ; set track for disk operation
    bne readsector    ; repeat until end of file

; Finish all up
; -------------
finish:
    lda #$3a          ; stepper back to normal speed
    sta $1c07
    lda $1c00         ; turn off DRIVE LED
    and #$F7
    sta $1c00
    rts               ; end of mission
