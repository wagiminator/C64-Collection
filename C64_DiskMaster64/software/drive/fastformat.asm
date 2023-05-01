; ====================================================================
; Project:   DiskMaster64 - Fast Format
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
; This implementation formats a disk.
;
; References:
; -----------
; Joe Forster/Michael Klein/Spiro Trikaliotis/Wolfgang Moser:
; cbmformat (part of OpenCBM: https://github.com/OpenCBM/OpenCBM)
;
; Assembling Instructions:
; ------------------------
; ca65 -l -t c64 fastformat.a65
; ld65 -t c64 -o fastformat.bin fastformat.o
;
; Operating Instructions:
; -----------------------
; "M-E"<addrLow><addrHigh><tracks><bump><clear><verify>:<name>,<id1><id2>
;
; $0200 - $0202 "M-E"       Memory Execute command
; $0203 - $0204 <addrL/H>   start address of this program in RAM ($0503)
; $0205         <tracks>    number of tracks to format
; $0206         <bump>      1 = perform head bump
; $0207         <clear>     1 = clear (demagnetize) disk
; $0208         <verify>    1 = verify disk after formatting
; $0209         ':'         separator
; $020a - ...   <name>      disk name (up to 16 characters)
;               ','         separator
;               <id1>       disk id1
;               <id2>       disk id2


TRACKS      = $0205   ; command buffer address for max track
BUMPFLAG    = $0206   ; command buffer address for bumb flag
CLEARFLAG   = $0207   ; command buffer address for clear flag
VERIFYFLAG  = $0208   ; command buffer address for verify flag
NAMEPOS     = $0a     ; index for disk name in command buffer
SECTORS     = $43     ; storage of number of sectors of current track
GAPLEN	    = $90     ; storage of current inter-sector gap length


.setcpu "6502"
.org $0500

    jmp diskformat    ; jump to format job (called by job loop)

; ====================================================================
; Start Routine (start program here)
; ====================================================================

; Initial checks
; --------------
start:
    lda $1c00         ; read port B
    and #$10          ; isolate bit for write protection
    beq error         ; 'WRITE PROTECT' -> finish
    lda TRACKS        ; get max track from command buffer
    cmp #41           ; track >= 41?
    bcs error         ; 'WRONG TRACK' -> finish

; Prepare disk operation
; ----------------------
    ldx #$01          ; set start track (#$01)
    stx $0a
    inx               ; set current buffer number (#$02)
    stx $f9
    jsr $c118         ; turn on DRIVE LED

; Bump the head
; -------------
    lda BUMPFLAG      ; get bump flag from command buffer
    beq parse         ; -> skip if zero
    lda #$c0          ; job code for head bump
    jsr $d58c         ; execute job

; Parse command line and set parameters
; -------------------------------------
parse:
    jsr success       ; send init status to adapter
    lda #$0b          ; command code for "NEW"
    sta $022a         ; set command code number
    jsr $c1ee         ; parse command line, find ':' and ','
    lda #NAMEPOS      ; move filename pointer behind ':'
    sta $027a
    ldy $027b         ; get comma position + 1
    lda $0200,y       ; set disk ID1
    sta $16
    lda $0201,y       ; set disk ID2
    sta $17
    lda #$05          ; set initial inter-sector gap
    sta GAPLEN

; Create empty sector data
; ------------------------
    lda #$04          ; set buffer pointer to $0400
    sta $31
    lda #$00          ; all zeros
    tay               ; set buffer index (#$00)
cesloop:
    sta ($30),y       ; clear the buffer
    iny               ; increase buffer index
    bne cesloop       ; repeat for 256 bytes
    sta $3a           ; set checksum (must also be zero)
    jsr $f78f         ; GCR encode block

; Move sector data in overflow buffer
; -----------------------------------
    ldy #$ba          ; overflow buffer index
mesloop:
    lda $0100,y       ; from $01ba-$01ff
    sta $0700,y       ; to   $07ba-$07ff
    iny               ; increase buffer index
    bne mesloop       ; repeat for 70 bytes

; Format tracks on disk, write BAM and finish
; -------------------------------------------
    lda #$e0          ; diskformat job at $0500
    jsr $d58c         ; execute job
    lda $02           ; read job status
    cmp #$01          ; was it successful?
    bne error         ; no -> 'WRITE ERROR'
    jsr $d00e         ; init
    jmp $ee40         ; create and write BAM, then quit

; Send status to adapter
; ----------------------
error:
    lda #$0a          ; 'ERROR': pull DATA and CLK low
    .byte $2c         ; skip next instruction
success:
    lda #$02          ; 'SUCCESS': only pull DATA low
    sta $1800         ; set status on IEC bus
    lsr
    sta $1800         ; release lines
    rts               ; return


; ====================================================================
; Job Routine (formats the disk)
; ====================================================================

diskformat:

; Get and set track parameters
; ----------------------------
    lda $0a           ; get current track
    ldx #$03          ; zone list index
gnsloop:
    cmp ZONES,x       ; compare with zone change values
    beq newzone       ; zone change? -> prepare new zone
    dex               ; decrease list index
    bpl gnsloop       ; check next zone
    bmi cleartrack    ; no zone change -> skip newzone

newzone:
    lda GAPADDS,x     ; get change of inter-sector gap length
    adc GAPLEN        ; add to current gap length (carry is set)
    sta GAPLEN        ; and store it
    lda $1c00         ; set zone speed
    and #$9f
    ora SPEEDS,x      ; speed mask of current zone
    sta $1c00
    lda $fed1,x       ; get number of sectors
    sta SECTORS       ; and store it here

; Demagnetize track
; -----------------
cleartrack:
    jsr trailgap      ; write trailing GAP
    lda CLEARFLAG     ; get clear flag from command buffer
    beq header        ; -> skip if zero
    jsr $fda3         ; overwrite track with #$ff
    jsr $fe0e         ; overwrite track with #$55

; Prepare header for all sectors
; ------------------------------
header:
;   ldy #$00          ; header pointer of sector 0
    ldx #$00          ; start with sector 0
headerloop:
    lda #$08          ; header block ID
    sta $0300,y       ; -> pos 0
    lda $0a           ; current track
    sta $0303,y       ; -> pos 3
    lda $17           ; 2nd character of disk ID
    sta $0304,y       ; -> pos 4
    lda $16           ; 1st character of disk ID
    sta $0305,y       ; -> pos 5
    lda #$0f	      ; header padding
    sta $0306,y       ; -> pos 6
    sta $0307,y       ; -> pos 7
    txa               ; current sector
    sta $0302,y       ; -> pos 2
    eor $0a           ; calculate checksum
    eor $17
    eor $16
    sta $0301,y       ; -> pos 1
    tya               ; increase header pointer by 8
    clc
    adc #$8
    tay
    inx               ; increase sector
    cpx SECTORS       ; max sector number reached?
    bcc headerloop    ; repeat for all sectors
    stx $0b           ; store as sector counter

; GCR encode sector headers
; -------------------------
    lda #$03          ; set buffer pointer to $0300
    sta $31
    jsr $fe30         ; GCR encode headers

; Move header data in memory
; --------------------------
    ldy #$ba
    jsr $fde5         ; move $0300-$03ba to $0345-$03ff
    jsr $fdf5         ; move $01bb-$01ff to $0300-$0344

; Write sectors to disk
; ---------------------
secstart:
    lda #$00          ; start with sector 0
    sta $32           ; pointer to current header data

; Write GAP and SYNC between sectors
; ----------------------------------
secloop:
    ldy GAPLEN        ; number of inter-sector GAP bytes
    jsr syncwrite     ; write GAP and SYNC

; Write sector header onto disk
; -----------------------------
    ldx #$0a          ; write 10 byte
    ldy $32           ; get pointer to current header data
wshloop:
    lda $0300,y       ; get header data byte
    bvc *             ; wait for end of previous byte
    clv               ; clear overflow flag
    sta $1c01         ; set byte for writing
    iny               ; increase header pointer
    dex               ; decrease loop counter
    bne wshloop       ; repeat for all 10 header bytes
    sty $32           ; store current header pointer

; Write GAP and SYNC between header and data block
; ------------------------------------------------
    ldy #$09          ; 9 bytes GAP
    jsr syncwrite     ; write GAP and SYNC

; Write GCR coded data block
; --------------------------
    ldy #$bb          ; index overflow buffer $07bb-$07ff
wdbloop1:
    lda $0700,y       ; get byte from overflow buffer
    bvc *             ; wait for previous byte to be written
    clv               ; clear overflow flag
    sta $1c01         ; set byte for writing
    iny               ; increase buffer index
    bne wdbloop1      ; repeat for 69 bytes
wdbloop2:
    lda $0400,y       ; get byte from data buffer
    bvc *             ; wait for previous byte to be written
    clv               ; clear overflow flag
    sta $1c01         ; set byte for writing
    iny               ; increase buffer index
    bne wdbloop2      ; repeat for 256 bytes

; Advance to next sector
; ----------------------
    dec $0b
    bne secloop       ; repeat for all sectors

; Finish writing
; --------------
    bvc *             ; wait for last byte to be written
    clv               ; clear overflow flag
    bvc *             ; wait for another byte to be written
    clv               ; clear overflow flag
    jsr $fe00         ; switch back to reading

; Test whether sectors are evenly distributed on track
; ----------------------------------------------------
    lda SECTORS       ; get number of sectors
    sta $0b           ; and reset the sector counter
    ldx GAPLEN        ; get current inter-sector gap length
    dex               ; subtract the 2 additional waited bytes
    dex
    dey               ; y=-1: marker/counter for excess gaps
testgap:              ; count the GAP bytes until SYNC of sector 0
    lda $1c00
    bpl syncfound     ; SYNC found?
    bvc testgap       ; wait for the next byte
    clv               ; clear overflow flag
    lda $1c01         ; clear received data byte
    dex               ; decrease GAP byte counter
    bne testgap       ; repeat
    iny               ; increase marker/counter for excess gaps
    ldx SECTORS       ; now put total number of sectors into x
    bne testgap       ; and continue counting

syncfound:
    stx $05           ; store the remaining gap for later testing
    clv               ; allow for accounting the next byte
    lda $1c01         ; clear received data byte
    sty $10           ; store additional GAP bytes to add
    ldx #$0a          ; read 10 header bytes
    ldy #$00          ; header pointer sector 0
cmpbyte:
    bvc *             ; wait for byte to be received
    clv               ; clear overflow flag
    lda $1c01         ; get received data byte
    cmp $0300,y       ; compare with expected header 0 data
    bne lessgap       ; header 0 overwritten -> GAPLEN was too big
    iny               ; increase header pointer
    dex               ; decrease bytes counter
    bne cmpbyte       ; repeat for all 10 header bytes
    sty $32           ; store header pointer for later

    lda $10           ; check number of additional GAP bytes per sector
    bpl moregap       ; increase gap if necessary
    ldx $05           ; get missing GAP bytes
    cpx #$02          ; just a few?
    bcc verify        ; -> can be ignored

lessgap:
    lda GAPLEN        ; get current gap length
    lsr               ; and half it
    bne storegap      ; if not zero, store it
    beq vererror      ; zero -> totally messed up

moregap:
    cmp #$02          ; just a few?
    bcc verify        ; -> can be ignored
    clc
    adc GAPLEN        ; add additional GAP bytes

storegap:
    sta GAPLEN        ; store new gap length
    jsr trailgap      ; switch to write mode, write trailing GAP
    jmp secstart      ; write track again with new gaps

; Verify track
; ------------
verify:
    lda VERIFYFLAG    ; get verify flag from command buffer
    beq handshake     ; -> skip if zero
    bne verifyblock   ; first header was already checked

verifyheader:
    jsr $f556         ; wait for SYNC
    ldx #$0a          ; check 10 header bytes
    ldy $32           ; get header pointer
vt01:
    bvc *             ; wait for byte to be received
    clv               ; clear overflow flag
    lda $1c01         ; get received data byte
    cmp $0300,y       ; compare with header data
    bne vererror      ; verification error?
    iny               ; increase buffer index
    dex               ; decrease loop counter
    bne vt01          ; repeat for all 10 header bytes
    sty $32           ; store current header pointer

verifyblock:
    jsr $f556         ; wait for SYNC
    ldy #$bb          ; index overflow buffer $07bb-$07ff
vt02:
    bvc *             ; wait for byte to be received
    clv               ; clear overflow flag
    lda $1c01         ; get received data byte
    cmp $0700,y       ; compare with overflow buffer
    bne vererror      ; verification error?
    iny               ; increase buffer index
    bne vt02          ; repeat 69 times
vt03:
    bvc *             ; wait for byte to be received
    clv               ; clear overflow flag
    lda $1c01         ; get received data byte
    cmp $0400,y       ; compare with data buffer
    bne vererror      ; verification error?
    iny               ; increase buffer index
    bne vt03          ; repeat 256 times

    dec $0b           ; decrease sector counter
    bne verifyheader  ; repeat for all sectors

; Send status to adapter
; ----------------------
handshake:
    jsr success       ; send 'TRACK SUCCESS'

; Advance to next track
; ---------------------
    lda $0a           ; get track number
    cmp TRACKS        ; last track written?
    bcs terminate     ; -> terminate job
    inc $0a           ; increment track number
    inc $22           ; track under r/w head
    ldy #$02          ; advance for 2 half-tracks
advloop1:
    jsr $fa63         ; advance half-track inwards
    ldx #$80          ; wait for r/w head to react
advloop2:
    jsr $fef3         ; delay about 40us
    dex               ; decrease delay counter
    bne advloop2      ; delay about 5ms (128*40us)
    dey               ; decrease half-track counter
    bne advloop1      ; repeat for 2 half-tracks
    jmp diskformat    ; proceed with next track

; Set return code and terminate job
; ---------------------------------
terminate:
    lda #$01          ; set return code 'OK'
    .byte $2c         ; skip next instruction
vererror:
    lda #$20          ; set return code 'ERROR'
    jmp $f969         ; terminate job


; ====================================================================
; Sub Routines
; ====================================================================

; Start writing trailing GAP
; --------------------------
trailgap:
    lda #$ce          ; change PCR
    sta $1c0c         ; to output
    lda #$ff          ; port A (read/write head)
    sta $1c03         ; to output
    ldy #$00          ; 256 times ...
    lda #$55          ; GAP byte (#$55 = #%01010101)
    bne writebytes    ; write them on disk

; Write y*GAP, then 5*SYNC
; ------------------------
syncwrite:
    lda #$55          ; GAP byte (#$55)
swloop:
    bvc *             ; wait for previous byte to be written
    clv               ; clear overflow flag
    sta $1c01         ; set byte for writing
    dey               ; decrease counter
    bne swloop        ; repeat for all bytes
    ldy #$05          ; bytes counter
    lda #$ff          ; SYNC byte (#$ff)

; Write y*accu to disk
; --------------------
writebytes:
    bvc *             ; wait for previous byte to be written
    clv               ; clear overflow flag
    sta $1c01         ; set byte for writing
    dey               ; decrease counter
    bne writebytes    ; repeat for y bytes
    rts

; Track zone parameters
; ---------------------
ZONES:    .byte 31, 25, 18, 1
SPEEDS:   .byte $00, $20, $40, $60
GAPADDS:  .byte $fc, $fa, $08, $ff
