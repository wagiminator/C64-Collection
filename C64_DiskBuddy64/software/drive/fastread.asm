; ====================================================================
; Project:   DiskBuddy64 - Fast IEC Implementation for 1541 - Reading
; Version:   v1.2
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
    bcs finish        ; 'WRONG TRACK' -> finish
    sta $0a           ; set track for disk operation
    lda #$00          ; sector index start value (#$00)
    sta $05           ; store in $05
    jsr $c118         ; turn on DRIVE LED
    lda #$12          ; speed up stepper
    sta $1c07
    jsr $c63d         ; check drive and initialize
    bne finish        ; 'READ ERROR' -> finish

; Read sectors from disk
; ----------------------
    ldx #$05          ; number of retries
retry:
    lda #$e0          ; read job at $0500
    sta $02           ; set job -> start disk operation
waitcomplete:
    lda $02           ; read job status
    bmi waitcomplete  ; wait for job to complete
    cmp #$01          ; was it successful?
    beq finish        ; -> finish
    dex               ; decrease retry counter
    bne retry         ; try again (max 5x)

; Finish all up
; -------------
finish:
    lda #$3a          ; stepper back to normal speed
    sta $1c07
    lda $1c00         ; turn off DRIVE LED
    and #$F7
    sta $1c00
    rts               ; end of mission


; ====================================================================
; Job Routine (reads list of sectors on track and sends them via IEC)
; ====================================================================

; Read sector from disk
; ---------------------
readjob:
    ldx $05           ; get sector index
    lda $0207,x       ; get sector from list in command buffer
    sta $0b           ; set sector for disk operation
    lda #$03          ; set buffer pointer:
    sta $31           ; -> $0300
    jsr $f50a         ; find beginning of block
wr01:
    bvc wr01          ; byte ready?
    clv
    lda $1c01         ; get data byte
    sta ($30),y       ; and write in buffer
    iny               ; increase buffer index
    bne wr01          ; repeat 256 times
    ldy #$ba          ; buffer index GCR buffer
wr02:
    bvc wr02          ; byte ready?
    clv
    lda $1c01         ; get data byte
    sta $0100,y       ; write in GCR buffer
    iny               ; ($01BA - $01FF)
    bne wr02          ; repeat 69 times
    jsr $f8e0         ; decode GCR
    jsr $f5e9         ; calculate parity
    cmp $3a           ; agreement?
    bne return        ; no -> 'READ ERROR'

; Send 256 bytes to adapter via fast IEC
; --------------------------------------
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

; Prepare next sector
; -------------------
    inc $05           ; increment sector index
    dec $0206         ; decrement number of sectors
    bne readjob       ; repeat for all sectors

; Set return code and terminate job
; ---------------------------------
    lda #$01          ; set return code 'OK'
    .byte $2c         ; skip next instruction
return:
    lda #$05          ; set return code 'READ ERROR'
    jmp $f969         ; finish job
