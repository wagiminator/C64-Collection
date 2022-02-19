; ====================================================================
; Project:   DumpMaster64 - Fast IEC Implementation for 1541 - Loading
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
; from there with the DumpMaster64 adapter.
; This implementation reads a file starting at the specified track
; and sector.
;
; References:
; -----------
; Michael Steil: https://www.pagetable.com/?p=568
;
; Assembling Instructions:
; ------------------------
; ca65 -l -t c64 fastload.a65
; ld65 -t c64 -o fastload.bin fastload.o
;
; Operating Instructions:
; -----------------------
; "M-E"<addrLow><addrHigh><track><sector>
;
; $0200 - $0202 "M-E"       Memory Execute command
; $0203 - $0204 <addrL/H>   start address of this program in RAM ($0503)
; $0205         <track>     track on disk to start reading
; $0206         <sector>    sector on track to start reading


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
    sta $0a           ; set track for disk operation
    lda $0206         ; get sector from command buffer
    sta $0b           ; set sector for disk operation
    jsr $c118         ; turn on DRIVE LED
    lda #$12          ; speed up stepper
    sta $1c07
    jsr $c63d         ; check drive and initialize
    bne error         ; 'INIT ERROR' -> finish

; Read sector from disk
; ---------------------
readsector:
    lda $0a           ; get current track
    cmp #41           ; track >= 41?
    bcs error         ; 'WRONG TRACK ERROR' -> finish
    ldx #$05          ; number of retries
retry:
    lda #$e0          ; read job at $0500
    sta $02           ; set job -> start disk operation
waitcomplete:
    lda $02           ; read job status
    bmi waitcomplete  ; wait for job to complete
    cmp #$01          ; was it successful?
    beq sendblock     ; -> send data block via IEC
    dex               ; decrease retry counter
    bne retry         ; try again (max 5x)

; Declare 'ERROR' and quit
; ------------------------
error:
    lda #$0a          ; declare 'ERROR':
    sta $1800         ; -> pull CLK and DATA LOW
    lsr
    sta $1800         ; release CLK and DATA
    bne finish        ; terminate reading

; Send 256 bytes to adapter via fast IEC
; --------------------------------------
sendblock:
    asl               ; declare 'START SENDING BLOCK':
    sta $1800         ; -> pull DATA LOW
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
    lda $0301         ; get next sector from block just read
    sta $0b           ; set sector for next disk operation
    lda $0300         ; get next track from block just read
    sta $0a           ; set track for next disk operation
    bne readsector    ; repeat until end of file

; Finish all up
; -------------
finish:
    lda #$3a          ; stepper back to normal speed
    sta $1c07
    rts               ; end of mission


; ====================================================================
; Job Routine (reads a sector from disk)
; ====================================================================

; Read sector from disk
; ---------------------
readjob:
    lda #$03          ; set buffer pointer:
    sta $31           ; -> $0300
    jsr $f50a         ; find beginning of block
wr01:
    bvc *             ; wait for byte to be read
    clv               ; clear overflow flag
    lda $1c01         ; get received data byte
    sta ($30),y       ; and write in data buffer ($0300 - $03ff)
    iny               ; increase buffer index
    bne wr01          ; repeat for 256 bytes
    ldy #$ba          ; buffer index overflow buffer
wr02:
    bvc *             ; wait for byte to be read
    clv               ; clear overflow flag
    lda $1c01         ; get received data byte
    sta $0100,y       ; write in overflow buffer ($01ba - $01ff)
    iny               ; increase buffer index
    bne wr02          ; repeat for 69 bytes
    jsr $f8e0         ; decode GCR
    jsr $f5e9         ; calculate parity
    cmp $3a           ; agreement?
    bne return        ; no -> 'READ ERROR'

; Set return code and terminate job
; ---------------------------------
    lda #$01          ; set return code 'OK'
    .byte $2c         ; skip next instruction
return:
    lda #$05          ; set return code 'READ ERROR'
    jmp $f969         ; finish job
