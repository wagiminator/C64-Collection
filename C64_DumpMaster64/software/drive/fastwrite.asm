; ====================================================================
; Project:   DumpMaster64 - Fast IEC Implementation for 1541 - Writing
; Version:   v1.0
; Year:      2022
; Author:    Stefan Wagner
; Github:    https://github.com/wagiminator
; EasyEDA:   https://easyeda.com/wagiminator
; License:   http://creativecommons.org/licenses/by-sa/3.0/
; ====================================================================
;
; Description:
; ------------
; Simple fast writing by accelerating the IEC communication due to
; the use of an asynchronous 2-bit parallel protocol. This program is
; loaded into the memory of the floppy disk drive and communicates
; from there with the DumpMaster64 adapter.
; This implementation writes a list of sectors to the specified track
; on the disk.
;
; References:
; -----------
; Michael Steil: https://www.pagetable.com/?p=568
;
; Assembling Instructions:
; ------------------------
; ca65 -l -t c64 fastwrite.a65
; ld65 -t c64 -o fastwrite.bin fastwrite.o
;
; Operating Instructions:
; -----------------------
; "M-E"<addrLow><addrHigh><track><#sectors><sector1><sector2>...
;
; $0200 - $0202 "M-E"       Memory Execute command
; $0203 - $0204 <addrL/H>   start address of this program in RAM ($0503)
; $0205         <track>     track on disk to write to
; $0206         <#sectors>  number of sectors in the following list
; $0207 - ...   <sectorX>   list of sectors to write in order


.setcpu "6502"
.org $0500

    jmp writejob      ; jump to write job (called by job loop)

; ====================================================================
; Start Routine (start program here)
; ====================================================================

; Initial setup
; -------------
start:
    lda $1c00         ; read port B
    and #$10          ; isolate bit for 'WRITE PROTECT'
    beq writeerror    ; 'WRITE PROTECT' -> finish
    lda $0205         ; get track from command buffer
    cmp #41           ; track >= 41?
    bcs finish        ; 'WRONG TRACK' -> finish
    sta $0a           ; set track for disk operation
    jsr $c118         ; turn on DRIVE LED
    lda #$12          ; speed up stepper
    sta $1c07
    jsr $c63d         ; check drive and initialize
    bne writeerror    ; 'WRITE ERROR' -> finish
    ldy #$00          ; sector index start value (#$00)

; Receive 256 bytes from adapter via fast IEC
; -------------------------------------------
receiveblock:
    lda #$01          ; wait for adapter 'READY TO SEND BLOCK':
waitready:
    bit $1800         ; test DATA line
    beq waitready     ; wait for DATA LOW
    sei               ; disable interrupts
    ldx #$00          ; buffer index
receivebyte:
    lda #$08          ; 2 declare 'READY TO RECEIVE BYTE':
    sta $1800         ; 4 -> pull CLK LOW
    lsr               ; 2 declare 'LETS GO':
    sta $1800         ; 4 -> release CLK HIGH
    lda $1800         ; 4 get bits 3 and 1
    asl               ; 2
    ora $1800         ; 4 get bits 2 and 0
    and #$0F          ; 2
    sta $05           ; 3 store low nibble
    lda $1800         ; 4 get bits 7 and 5
    asl               ; 2
    ora $1800         ; 4 get bits 6 and 4
    asl               ; 2
    asl               ; 2
    asl               ; 2
    asl               ; 2 high nibble
    ora $05           ; 3 -> combine with low nibble
    sta $0300,x       ; 5 -> and write it to buffer
    inx               ; 2 increase buffer index
    bne receivebyte   ; 3 repeat for all 256 bytes
    cli               ; enable interrupts

; Write sector to disk
; --------------------
    stx $05           ; clear encoded flag for write job
    lda $0207,y       ; get sector from list in command buffer
    sta $0b           ; set sector for disk operation
    ldx #$03          ; number of retries
retry:
    lda #$e0          ; write job at $0500
    sta $02           ; set job -> start disk operation
waitcomplete:
    lda $02           ; read job status
    bmi waitcomplete  ; wait for job to complete
    cmp #$01          ; was it successful?
    beq nextsector    ; -> go on with next sector
    dex               ; decrease retry counter
    bne retry         ; try again (max 3x)

; Declare 'WRITE ERROR' and finish
; --------------------------------
writeerror:
    ldx #$0a          ; return code 'WRITE ERROR'
    bne finish        ; terminate job

; Prepare next sector
; --------------------
nextsector:
    iny               ; increment sector index
    dec $0206         ; decrement number of sectors left
    bne receiveblock  ; repeat for all sectors
    ldx #$08          ; return code 'SUCCESS'

; Finish all up
; -------------
finish:
    lda #$3a          ; stepper back to normal speed
    sta $1c07
    lda #$01          ; wait for adapter 'READY':
waitadp:
    bit $1800         ; test DATA line
    beq waitadp       ; wait for DATA LOW
    stx $1800         ; set return code
    sta $1800         ; release all lines
    lda $1c00         ; turn off DRIVE LED
    and #$F7
    sta $1c00
    rts               ; end of mission


; ====================================================================
; Job Function (writes a sector on the specified track)
; ====================================================================

; GCR encode data block
; ---------------------
writejob:
    lda #$03          ; set buffer pointer:
    sta $31           ; -> $0300
    lda $05           ; data already GCR encoded?
    bne find          ; -> skip encoding
    jsr $f5e9         ; calculate parity for buffer
    sta $3a           ; and save
    jsr $f78f         ; encode GCR
    inc $05           ; set encoded flag

; Find block header
; -----------------
find:
    jsr $f510         ; find block header

; Skip 9 bytes (GAP)
; ------------------
    ldx #$09          ; 9 bytes gap after header
gaploop:
    bvc gaploop       ; byte ready?
    clv
    dex               ; decrease GAP byte counter
    bne gaploop       ; skip 9 GAP bytes

; Switch head to write mode
; -------------------------
    lda #$ff          ; port A (read/write head)
    sta $1c03         ; to output
    lda $1c0c         ; change PCR to output
    and #$1f
    ora #$c0
    sta $1c0c

; Write 5 times SYNC byte (#$ff)
; ------------------------------
    lda #$ff          ; SYNC byte: #$ff
    ldx #$05          ; SYNC byte counter: 5 times
    sta $1c01         ; write to disk
    clv
syncloop:
    bvc syncloop      ; wait for SYNC byte written
    clv
    dex               ; decrease SYNC byte conter
    bne syncloop      ; repeat for 5 SYNC bytes

; Write GCR coded data block
; --------------------------
    ldy #$bb          ; bytes $01bb bis $01ff
gcrloop:
    lda $0100,y       ; write GCR buffer (69 bytes)
wr01:
    bvc wr01          ; wait for ready
    clv
    sta $1c01         ; write byte
    iny               ; increase buffer index
    bne gcrloop       ; repeat for 69 bytes

dataloop:
    lda ($30),y       ; write data buffer (256 bytes)
wr02:
    bvc wr02          ; wait for ready
    clv
    sta $1c01         ; write byte
    iny               ; increase buffer index
    bne dataloop      ; repeat for 256 bytes

; Switch head back to read mode
; -----------------------------
wr03:
    bvc wr03          ; byte ready?
    lda $1c0c         ; PCR to input again
    ora #$e0
    sta $1c0c
    lda #$00          ; port A (read/write head)
    sta $1c03         ; to input

; Set return code and terminate job
; ---------------------------------
    lda #$01          ; return code 'OK'
    jmp $f969         ; finish job
