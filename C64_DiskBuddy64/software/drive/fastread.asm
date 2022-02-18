; ====================================================================
; Project:   DiskBuddy64 - Fast IEC Implementation for 1541 - Reading
; Version:   v1.5
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
; This implementation reads a list of sectors on the specified track.
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
; "M-E"<addrLow><addrHigh><track><#sectors><sector1><sector2>...
;
; $0200 - $0202 "M-E"       Memory Execute command
; $0203 - $0204 <addrL/H>   start address of this program in RAM ($0503)
; $0205         <track>     track on disk to read from
; $0206         <#sectors>  number of sectors in the following list
; $0207 - ...   <sectorX>   list of sectors to read in order


.setcpu "6502"
.org $0500

    jmp readjob       ; jump to read job (called by job loop)

; ====================================================================
; Start Routine (start program here)
; ====================================================================

; Initial setup
; -------------
start:
    lda $0205         ; get track from command buffer
    cmp #41           ; track >= 41?
    bcs finish        ; 'WRONG TRACK ERROR' -> finish
    sta $0a           ; set track for disk operation
    lda #$00          ; sector index start value (#$00)
    sta $05           ; store in $05
    jsr $c118         ; turn on DRIVE LED
    lda #$12          ; speed up stepper
    sta $1c07
    jsr $c63d         ; check drive and initialize
    bne finish        ; 'INIT ERROR' -> finish

; Read sectors from disk
; ----------------------
    lda #$e0          ; read job at $0500
    sta $02           ; set job -> start disk operation
waitcomplete:
    lda $02           ; read job status
    bmi waitcomplete  ; wait for job to complete

; Finish all up
; -------------
finish:
    lda #$3a          ; stepper back to normal speed
    sta $1c07
    rts               ; end of mission


; ====================================================================
; Fast IEC Function (sends one data byte)
; ====================================================================

sendbyte:
    ldx #$08          ; 2 mark 'START SENDING BYTE':
    stx $1800         ; 4 -> pull CLK LOW
    tax               ; 2 save original in x
    lsr               ; 2 get high nibble
    lsr               ; 2
    lsr               ; 2
    lsr               ; 2
    sta $1800         ; 4 transfer bit 7 and 5
    asl               ; 2
    and #$0F          ; 2
    sta $1800         ; 4 transfer bit 6 and 4
    txa               ; 2 get original byte
    and #$0F          ; 2 get low nibble
    sta $1800         ; 4 transfer bit 3 and 1
    asl               ; 2
    and #$0F          ; 2
    sta $1800         ; 4 transfer bit 2 and 0
    ldx #$00          ; 2 mark 'END OF BYTE':
    stx $1800         ; 4 -> release CLK HIGH
    rts               ; 6 return


; ====================================================================
; Job Routine (reads list of sectors on track and sends them via IEC)
; ====================================================================

; Read sector from disk
; ---------------------
readjob:
    ldy $05           ; get sector index
    lda $0207,y       ; get sector from list in command buffer
    sta $0b           ; set sector for disk operation
    jsr $f50a         ; find beginning of block
wr01:
    bvc *             ; wait for byte to be read
    clv               ; clear overflow flag
    lda $1c01         ; get received byte
    sta $0300,y       ; and write into data buffer ($0300 - $03ff)
    iny               ; increase buffer index
    bne wr01          ; repeat for 256 bytes
    ldy #$bb          ; index overflow buffer
wr02:
    bvc *             ; wait for byte to be read
    clv               ; clear overflow flag
    lda $1c01         ; get received byte
    sta $0100,y       ; write into overflow buffer ($01bb - $01ff)
    iny               ; increase buffer index
    bne wr02          ; repeat for 69 bytes

; Send GCR-encoded block (325 bytes) to adapter via fast IEC
; ----------------------------------------------------------
sl01:
    lda $0300,y       ; read byte from data buffer ($0300 - $03ff)
    jsr sendbyte      ; send byte via fast IEC
    iny               ; increase buffer index
    bne sl01          ; repeat for 256 bytes
    ldy #$bb          ; index overflow buffer
sl02:
    lda $0100,y       ; read byte from overflow buffer ($01bb - $01ff)
    jsr sendbyte      ; send byte via fast IEC
    iny               ; increase buffer index
    bne sl02          ; repeat for 69 bytes

; Prepare next sector
; -------------------
    inc $05           ; increment sector index
    dec $0206         ; decrement number of sectors
    bne readjob       ; repeat for all sectors

; Set return code and terminate job
; ---------------------------------
    lda #$01          ; set return code 'OK'
    jmp $f969         ; finish job
