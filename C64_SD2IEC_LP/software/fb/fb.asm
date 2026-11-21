; ===================================================================================
; FB - File Browser for the SD2IEC                                          * v1.0 *
; ===================================================================================
;
; A file browser for the C64 that works with the SD2IEC and every other drive:
; browse folders and disk images on the SD card, start programs with a key press,
; copy files between drives and transfer disk images to and from real floppies.
;
; Keys (joystick in port 2 works as well):
; -----------------------------------------
; CRSR up/down    move the selection         FIRE / RETURN   open / start
; CRSR left/right page up/down               <- or DEL       back (parent folder)
; HOME            first entry                /               root folder
; A..Z, 0..9      jump to the next entry with this first letter
; F1              next drive (8..15)         F3              read the folder again
; F5              copy the file to another drive
; F7              disk image <-> floppy disk (whole disk, track by track)
; @               send a DOS command (e.g. "S:NAME", "N:DISK,ID", "CD:GAMES")
; RUN/STOP        quit to BASIC
;
; Notes:
; ------
; - Assembles with 64tass or with the C64 Assembler in the browser:
;   https://wagiminator.github.io/apps/c64-assembler/
; - Programs are started like LOAD"NAME",8,1 followed by RUN. Programs that do not
;   load to the BASIC start ($0801) get a SYS line at the screen instead.
; - Memory: code $0801-$1FFF, folder table $2000-$9FFF (1638 entries),
;   copy buffer $A000-$CFFF (BASIC ROM is switched off while FB runs).
;
; 2026 by Stefan Wagner:   https://github.com/wagiminator

; ===================================================================================
; Definitions
; ===================================================================================

; KERNAL
SETLFS  = $ffba
SETNAM  = $ffbd
OPEN    = $ffc0
CLOSE   = $ffc3
CHKIN   = $ffc6
CHKOUT  = $ffc9
CLRCHN  = $ffcc
CHRIN   = $ffcf
CHROUT  = $ffd2
LOAD    = $ffd5
GETIN   = $ffe4
CLALL   = $ffe7
READST  = $ffb7

; Memory
SCREEN  = $0400                 ; screen RAM
COLRAM  = $d800                 ; colour RAM
TABLE   = $2000                 ; folder table: 20 bytes per entry
TABEND  = $a000
ENTSIZE = 20                    ; name (16), type, blocks (2), name length
MAXENT  = (TABEND - TABLE) / ENTSIZE
BUFFER  = $a000                 ; copy buffer (RAM under the BASIC ROM)
BUFEND  = $d000
ROWS    = 20                    ; entries on the screen
ROW0    = 3                     ; first screen row of the list

; Entry fields
E_TYPE  = 16                    ; type 0..6 (DEL SEQ PRG USR REL CBM DIR), bit 7: image
E_BLKS  = 17                    ; blocks (2 bytes)
E_LEN   = 19                    ; length of the name

; Colours
C_FRAME = 11                    ; dark grey: border and lines
C_TITLE = 14                    ; light blue: title bar and selection
C_HEAD  = 15                    ; light grey: disk name
C_DIR   = 7                     ; yellow: folders
C_IMAGE = 13                    ; light green: disk images
C_PRG   = 1                     ; white: programs
C_OTHER = 12                    ; grey: other files
C_STAT  = 15                    ; light grey: drive status
C_HELP  = 12                    ; grey: help line

; Zero page (BASIC work area, free while FB runs)
ptr     = $fb                   ; pointer to an entry
ptr2    = $fd                   ; screen / general pointer
scr     = $57                   ; screen pointer
col     = $59                   ; colour RAM pointer
count   = $5b                   ; number of entries (2 bytes)
top     = $5d                   ; first entry on the screen (2 bytes)
cur     = $5f                   ; selected entry (2 bytes)
idx     = $61                   ; index for drawing (2 bytes)
num     = $63                   ; number for printing / from the drive (2 bytes)
tmp     = $65
tmp2    = $66
state   = $67                   ; parser state
eof     = $68                   ; end of the input (status bits)
color   = $69                   ; colour for printing
bptr    = $6a                   ; buffer pointer (2 bytes)

; ===================================================================================
; Start
; ===================================================================================

        * = $0801
        .basic                  ; 10 SYS 2061

start   tsx                     ; for quitting from anywhere
        stx savesp
        lda $ba                 ; the drive FB was loaded from
        cmp #8
        bcs +
        lda #8
+       sta dev
        lda #$36                ; BASIC ROM off: RAM at $A000-$BFFF
        sta $01
        lda #0                  ; C= + SHIFT switches to lower case (long names)
        sta $0291
        lda #0
        sta $c6                 ; empty keyboard buffer
        sta depth
        sta joylast
        lda #$15                ; upper case characters
        sta $d018
        lda #0
        sta $d021
        lda #C_FRAME
        sta $d020
        jsr frame               ; screen layout
        jsr readdir             ; first folder

; ===================================================================================
; Main Loop
; ===================================================================================

main    jsr drawall
mainkey jsr waitkey             ; key or joystick in A
        ldx #0
-       lda keytab,x            ; search the key table
        beq letter
        cmp key
        beq +
        inx
        inx
        inx
        bne -
+       lda keytab+2,x          ; call the handler
        pha
        lda keytab+1,x
        pha
        rts                     ; jumps to the handler (address - 1 in the table)

letter  lda key                 ; A..Z, 0..9: jump to an entry
        cmp #"0"
        bcc +
        cmp #"9"+1
        bcc ++
        cmp #"a"
        bcc +
        cmp #"z"+1
        bcc ++
+       jmp mainkey
+       jsr findletter
        jmp main

; Key table: key code, handler address - 1
keytab  .byte $11
        .word down-1            ; CRSR down
        .byte $91
        .word up-1              ; CRSR up
        .byte $1d
        .word pagedown-1        ; CRSR right
        .byte $9d
        .word pageup-1          ; CRSR left
        .byte $13
        .word home-1            ; HOME
        .byte $0d
        .word open-1            ; RETURN
        .byte $5f
        .word back-1            ; <-
        .byte $14
        .word back-1            ; DEL
        .byte $2f
        .word root-1            ; /
        .byte $85
        .word nextdrive-1       ; F1
        .byte $86
        .word reload-1          ; F3
        .byte $87
        .word copyfile-1        ; F5
        .byte $88
        .word diskcopy-1        ; F7
        .byte $40
        .word command-1         ; @
        .byte $03
        .word quit-1            ; RUN/STOP
        .byte 0

; ===================================================================================
; Navigation
; ===================================================================================

down    jsr inccur              ; next entry
        jsr visible
        jmp main

up      jsr deccur              ; previous entry
        jsr visible
        jmp main

pageup  ldx #ROWS               ; one page back: selection and list
pgup1   jsr deccur
        bcs pgupend
        lda top                 ; scroll if the list is not at the top
        ora top+1
        beq pgupnext
        lda top
        bne +
        dec top+1
+       dec top
pgupnext
        dex
        bne pgup1
pgupend jsr visible
        jmp main

pagedown
        ldx #ROWS               ; one page forward: selection and list
pgdn1   jsr inccur
        bcs pgdnend
        lda top                 ; scroll if the list continues below
        clc
        adc #ROWS
        sta tmp
        lda top+1
        adc #0
        sta tmp2
        lda tmp
        cmp count
        lda tmp2
        sbc count+1
        bcs pgdnnext
        inc top
        bne pgdnnext
        inc top+1
pgdnnext
        dex
        bne pgdn1
pgdnend jsr visible
        jmp main

home    lda #0
        sta cur
        sta cur+1
        sta top
        sta top+1
        jmp main

; cur + 1 if there is a next entry (carry clear), else carry set
inccur  lda cur
        clc
        adc #1
        sta tmp
        lda cur+1
        adc #0
        sta tmp2
        lda tmp                 ; cur + 1 < count?
        cmp count
        lda tmp2
        sbc count+1
        bcs +
        lda tmp
        sta cur
        lda tmp2
        sta cur+1
        clc
+       rts

; cur - 1 if cur > 0 (carry clear), else carry set
deccur  lda cur
        ora cur+1
        bne +
        sec                     ; cur = 0
        rts
+       lda cur
        bne +
        dec cur+1
+       dec cur
        clc
        rts

; Make sure the selected entry is on the screen (top <= cur < top + ROWS)
visible lda cur                 ; cur < top: top = cur
        cmp top
        lda cur+1
        sbc top+1
        bcs +
        lda cur
        sta top
        lda cur+1
        sta top+1
        rts
+       lda top                 ; cur >= top + ROWS: top = cur - ROWS + 1
        clc
        adc #ROWS
        sta tmp
        lda top+1
        adc #0
        sta tmp2
        lda cur
        cmp tmp
        lda cur+1
        sbc tmp2
        bcc +
        lda cur
        sec
        sbc #ROWS-1
        sta top
        lda cur+1
        sbc #0
        sta top+1
+       rts

; Jump to the next entry that starts with the letter in A
findletter
        sta letter2
        lda cur
        sta idx
        lda cur+1
        sta idx+1
        lda count               ; at most count entries
        sta num
        lda count+1
        sta num+1
flnext  lda num
        ora num+1
        beq flend
        lda num
        bne +
        dec num+1
+       dec num
        inc idx                 ; next entry (with wrap around)
        bne +
        inc idx+1
+       lda idx
        cmp count
        lda idx+1
        sbc count+1
        bcc +
        lda #0
        sta idx
        sta idx+1
+       jsr entptr
        ldy #0
        lda (ptr),y
        and #$7f                ; upper and lower case
        cmp letter2
        bne flnext
        lda idx
        sta cur
        lda idx+1
        sta cur+1
        jsr visible
flend   rts

; ===================================================================================
; Folders and Programs
; ===================================================================================

; RETURN / FIRE: open the selected entry
open    lda count
        ora count+1
        bne +
        jmp main
+       lda cur
        sta idx
        lda cur+1
        sta idx+1
        jsr entptr
        ldy #E_TYPE
        lda (ptr),y
        bmi enter               ; disk image: mount it
        cmp #6
        beq enter               ; folder
        cmp #2
        bne +
        jmp launch              ; program
+       jmp main                ; nothing to do for other files

; Change into the selected folder or image
enter   ldx depth               ; remember the position
        cpx #16
        bcs +
        lda cur
        sta histlo,x
        lda cur+1
        sta histhi,x
        lda top
        sta histtlo,x
        lda top+1
        sta histthi,x
+       inc depth
        jsr cmdcd               ; "CD:" + name
        ldy #E_LEN
        lda (ptr),y
        tax
        ldy #0
-       lda (ptr),y
        sta cmdbuf+3,y
        iny
        dex
        bne -
        tya
        clc
        adc #3
        jsr doscmd
        jsr readdir
        jmp main

; <- / DEL: back to the parent folder (or out of the image)
back    jsr cmdcd
        lda #$5f                ; "CD:<-"
        sta cmdbuf+3
        lda #4
        jsr doscmd
        jsr readdir
        lda depth               ; restore the position
        beq +
        dec depth
        ldx depth
        cpx #16
        bcs +
        lda histlo,x
        sta cur
        lda histhi,x
        sta cur+1
        lda histtlo,x
        sta top
        lda histthi,x
        sta top+1
        jsr clampcur
+       jmp main

; /: root folder
root    lda #"/"                ; "CD//"
        sta cmdbuf+2
        sta cmdbuf+3
        lda #0
        sta depth
        jsr cmdcd2
        lda #4
        jsr doscmd
        jsr readdir
        jmp main

; Put "CD:" into the command buffer
cmdcd   lda #":"
        sta cmdbuf+2
cmdcd2  lda #"c"
        sta cmdbuf
        lda #"d"
        sta cmdbuf+1
        rts

; Keep the selection inside the folder
clampcur
        lda cur
        cmp count
        lda cur+1
        sbc count+1
        bcc +
        lda #0
        sta cur
        sta cur+1
        sta top
        sta top+1
+       rts

; F3: read the folder again
reload  jsr readdir
        jmp main

; F1: next drive that is present (8..15)
nextdrive
        ldx #8                  ; try 8 drives
-       inc dev
        lda dev
        cmp #16
        bcc +
        lda #8
        sta dev
+       txa
        pha
        jsr status
        pla
        tax
        lda eof
        bpl +                   ; drive answered
        dex
        bne -
+       lda #0
        sta depth
        jsr frame
        jsr readdir
        jmp main

; RUN/STOP: back to BASIC
quit    jsr restore
        lda #147
        jsr CHROUT
        ldx savesp
        txs
        rts

; Restore the machine for BASIC
restore jsr CLALL
        lda #$37
        sta $01
        lda #0
        sta $0291
        sta $c6
        lda #14
        sta $d020
        sta $0286
        lda #6
        sta $d021
        rts

; ===================================================================================
; Start a Program
; ===================================================================================
; The loader runs in the cassette buffer, as the program overwrites FB.

launch  ldy #E_LEN              ; name into the loader
        lda (ptr),y
        sta namelen
        sta lnamelen
        tax
        ldy #0
-       lda (ptr),y
        sta lname,y
        sta name,y
        iny
        dex
        bne -
        lda dev
        sta ldev
        jsr getaddr             ; load address of the program
        ldx #0                  ; copy the loader
-       lda stub,x
        sta $0334,x
        inx
        cpx #stubend-stub
        bne -
        jsr restore
        ldx #0                  ; clear the screen and show the name
-       lda loadtxt,x
        beq +
        jsr CHROUT
        inx
        bne -
+       ldx #0
-       lda lname,x
        jsr CHROUT
        inx
        cpx lnamelen
        bne -
        lda #13
        jsr CHROUT
        ldx savesp              ; stack of BASIC
        txs
        jmp $0334

loadtxt .text 147, 13, "loading ", 0

; Read the load address of the file "name" into laddr
getaddr lda namelen
        ldx #<name
        ldy #>name
        jsr SETNAM
        lda #3
        ldx dev
        ldy #0
        jsr SETLFS
        lda #1                  ; default: BASIC
        sta laddrc
        lda #8
        sta laddrc+1
        jsr OPEN
        bcs +
        ldx #3
        jsr CHKIN
        bcs +
        lda #0
        sta eof
        jsr getbyte
        sta tmp
        jsr getbyte
        ldx eof
        cpx #$40                ; complete (or EOI at the second byte)
        bcs +
        ldx tmp
        stx laddrc
        sta laddrc+1
+       jsr CLRCHN
        lda #3
        jmp CLOSE

; The loader (copied to $0334)
stub    .logical $0334
        lda lnamelen2
        ldx #<lname2
        ldy #>lname2
        jsr SETNAM
        lda #1
        ldx ldev2
        ldy #1
        jsr SETLFS
        lda #0                  ; load to the address of the file
        jsr LOAD
        bcs lerror
        lda laddr               ; BASIC program at $0801?
        cmp #$01
        bne lsys
        lda laddr+1
        cmp #$08
        bne lsys
        stx $2d                 ; end of the program
        sty $2e
        jsr $a659               ; CLR
        jsr $a533               ; relink the lines
        jsr $a68e               ; start of the program
        jmp $a7ae               ; RUN
lsys    lda #13                 ; machine code: "SYS address" at the screen
        jsr CHROUT
        lda #"s"
        jsr CHROUT
        lda #"y"
        jsr CHROUT
        lda #"s"
        jsr CHROUT
        ldx laddr
        lda laddr+1
        jsr $bdcd               ; print the address
        ldx #0
-       lda readytxt,x
        jsr CHROUT
        inx
        cpx #7
        bne -
        sec                     ; cursor to the start of the SYS line
        jsr $fff0
        dex
        ldy #0
        clc
        jsr $fff0
        jmp ($0302)             ; BASIC input loop, RETURN starts the program
lerror  tax                     ; BASIC error message
        jmp $a437
readytxt .text 13, "ready."
ldev2   .byte 8
laddr   .word $0801
lnamelen2 .byte 0
lname2  .fill 16, 0
        .endlogical
stubend

lname   = lname2 - $0334 + stub ; the variables in the copy that is moved
lnamelen = lnamelen2 - $0334 + stub
ldev    = ldev2 - $0334 + stub
laddrc  = laddr - $0334 + stub

; ===================================================================================
; Reading the Folder
; ===================================================================================
; The folder is read like LOAD"$",8 and parsed line by line into the table.

readdir lda #0
        sta count
        sta count+1
        sta free
        sta free+1
        sta cur
        sta cur+1
        sta top
        sta top+1
        sta lineno
        ldx #21                 ; no disk name yet
        lda #" "
-       sta diskname,x
        dex
        bpl -
        jsr showbusy
        lda #1
        ldx #<dollar
        ldy #>dollar
        jsr SETNAM
        lda #2
        ldx dev
        ldy #0
        jsr SETLFS
        jsr OPEN
        bcs rderr
        ldx #2
        jsr CHKIN
        bcs rderr
        lda #0
        sta eof
        jsr getbyte             ; load address
        jsr getbyte
        lda eof
        bne rdend

rdline  jsr getbyte             ; link (0: end)
        sta tmp
        jsr getbyte
        ora tmp
        beq rdend
        jsr getbyte             ; number (blocks)
        sta num
        jsr getbyte
        sta num+1
        lda eof
        bne rdend
        jsr parseline           ; text
        inc lineno
        bne +
        dec lineno              ; (stays > 0)
+       lda eof
        bne rdend
        lda count+1             ; table full?
        cmp #>MAXENT
        bne +
        lda count
        cmp #<MAXENT
        beq rdend
+       jsr $ffe1               ; RUN/STOP aborts
        bne rdline

rdend   jsr CLRCHN
        lda #2
        jsr CLOSE
        jsr status              ; drive status
        jsr clampcur
        rts

rderr   jsr CLRCHN
        lda #2
        jsr CLOSE
        jsr status
        rts

; Read a byte, set eof if the input ended (A is kept)
getbyte jsr CHRIN
        pha
        jsr READST
        and #$c3                ; EOI, timeout, device not present
        ora eof
        sta eof
        pla
        rts

; Parse the text of a line: header, entry or BLOCKS FREE
parseline
        lda #0
        sta state               ; 0: before the name, 1: name, 2: after the name
        sta namelen
        sta typelen
plnext  lda eof
        bne pldone
        jsr getbyte
        cmp #0
        beq pldone              ; end of the line
        ldx state
        beq plbefore
        dex
        beq plname
        ldx typelen             ; after the name: id (header) or type
        bne +
        cmp #" "                ; (leading spaces skipped)
        beq plnext
+       cpx #5
        bcs plnext
        sta typetxt,x
        inc typelen
        bne plnext
plbefore
        cmp #$22                ; quote: name starts
        bne plnext
        inc state
        bne plnext
plname  cmp #$22                ; quote: name ends
        bne +
        inc state
        bne plnext
+       ldx namelen             ; name character
        cpx #16
        bcs plnext
        sta name,x
        inc namelen
        bne plnext

pldone  lda state               ; what was it?
        bne +
        lda num                 ; no quotes: BLOCKS FREE
        sta free
        lda num+1
        sta free+1
        rts
+       lda lineno              ; first line: disk name and id
        bne addentry
        ldx #15
-       lda name,x
        cpx namelen
        bcc +
        lda #" "
+       sta diskname,x
        dex
        bpl -
        ldx #4
-       lda typetxt,x
        cpx typelen
        bcc +
        lda #" "
+       sta diskname+17,x
        dex
        bpl -
        rts

; Add an entry to the table
addentry
        lda count
        sta idx
        lda count+1
        sta idx+1
        jsr entptr
        ldy #15                 ; name, padded with 0
-       lda name,y
        cpy namelen
        bcc +
        lda #0
+       sta (ptr),y
        dey
        bpl -
        ldy #E_LEN
        lda namelen
        sta (ptr),y
        ldy #E_BLKS
        lda num
        sta (ptr),y
        iny
        lda num+1
        sta (ptr),y
        jsr typecode            ; type from the text (skips "*" and spaces)
        ldy #E_TYPE
        sta (ptr),y
        inc count
        bne +
        inc count+1
+       rts

; Type code of the type text (0..6, 2 = PRG if unknown), bit 7 for disk images
typecode
        ldx #0                  ; first letter of the type ("*" for open files)
tcskip  cpx typelen
        bcs tcprg
        lda typetxt,x
        cmp #"a"
        bcc +
        cmp #"z"+1
        bcc tcfound
+       inx
        bne tcskip
tcfound stx tmp                 ; compare with the type names
        ldy #0
tcloop  ldx tmp
        lda typetxt,x
        cmp typenames,y
        bne tcnext
        lda typetxt+1,x
        cmp typenames+1,y
        bne tcnext
        lda typetxt+2,x
        cmp typenames+2,y
        beq tchit
tcnext  iny
        iny
        iny
        cpy #21
        bcc tcloop
tcprg   ldy #6                  ; unknown: PRG
tchit   tya                     ; y / 3 = code
        ldy #0
-       cmp #3
        bcc +
        sbc #3
        iny
        bne -
+       tya
        cmp #2                  ; PRG: disk image?
        bne +
        jsr isimage
+       rts

typenames .text "delseqprgusrrelcbmdir"

; PRG with a disk image name (.D64 .D71 .D81 .DNP .M2I)? Returns $82 or 2
isimage ldx namelen
        cpx #5
        bcc notimg
        lda name-4,x
        cmp #"."
        bne notimg
        lda name-3,x
        and #$7f
        cmp #"d"
        beq +
        cmp #"m"
        bne notimg
+       lda #$82
        rts
notimg  lda #2
        rts

; ptr = TABLE + idx * 20
entptr  lda idx                 ; idx * 4
        asl
        sta ptr
        lda idx+1
        rol
        sta ptr+1
        asl ptr
        rol ptr+1
        lda ptr                 ; + idx * 16
        sta tmp
        lda ptr+1
        sta tmp2
        asl tmp
        rol tmp2
        asl tmp
        rol tmp2
        lda ptr
        clc
        adc tmp
        sta ptr
        lda ptr+1
        adc tmp2
        adc #>TABLE
        sta ptr+1
        rts

dollar  .text "$"

; ===================================================================================
; Drive Status and DOS Commands
; ===================================================================================

; Send the command in cmdbuf (length in A) and read the status
doscmd  ldx #<cmdbuf
        ldy #>cmdbuf
        jmp +

; Read the status (empty command)
status  lda #0
+       jsr SETNAM
        lda #15
        ldx dev
        ldy #15
        jsr SETLFS
        lda #0
        sta eof
        jsr OPEN
        bcs stnone
        ldx #15
        jsr CHKIN
        bcs stnone
        jsr READST
        bmi stnone
        ldx #0
-       stx tmp
        jsr getbyte
        ldx tmp
        cmp #13
        beq +
        sta stattxt,x
        inx
        cpx #38
        bcs +
        lda eof
        beq -
+       lda #0
        sta stattxt,x
        jsr CLRCHN
        lda #15
        jsr CLOSE
        rts
stnone  jsr CLRCHN              ; device not present
        lda #15
        jsr CLOSE
        lda #$80
        sta eof
        ldx #0
-       lda notpresent,x
        sta stattxt,x
        beq +
        inx
        bne -
+       rts

notpresent .text "drive not present", 0

; @: enter and send a DOS command
command lda #23                 ; input line at the status row
        jsr input
        bcs +                   ; cancelled
        lda inlen
        beq +
        ldx inlen               ; copy to the command buffer
        dex
-       lda inbuf,x
        sta cmdbuf,x
        dex
        bpl -
        lda inlen
        jsr doscmd
        jsr savestat
        jsr readdir             ; the folder may have changed
        jsr restat
+       jmp main

; Keep the status of the command while the folder is read
savestat
        ldx #39
-       lda stattxt,x
        sta inbuf,x
        dex
        bpl -
        rts
restat  ldx #39
-       lda inbuf,x
        sta stattxt,x
        dex
        bpl -
        rts

; Line input at row A (returns carry set if cancelled)
input   jsr rowptr
        lda #0
        sta inlen
inloop  ldy #0                  ; draw prompt and text
        lda #"@"-$40
        sta (scr),y
        lda #1
        sta (col),y
        iny
-       ldx #" "
        tya
        sec
        sbc #1
        cmp inlen
        bcs +
        tax
        lda inbuf,x
        jsr petscr
        tax
+       txa
        sta (scr),y
        lda #1
        sta (col),y
        iny
        cpy #40
        bcc -
        ldy inlen               ; cursor
        iny
        cpy #40
        bcs +
        lda (scr),y
        ora #$80
        sta (scr),y
+       jsr GETIN
        beq +
        cmp #13
        beq inok
        cmp #3
        beq incancel
        cmp #$14                ; DEL
        bne ++
        lda inlen
        beq +
        dec inlen
+       jmp inloop
+       cmp #$20                ; printable characters
        bcc inloop
        cmp #$80
        bcc +
        cmp #$a0
        bcc inloop
+       ldx inlen
        cpx #38
        bcs inloop
        sta inbuf,x
        inc inlen
        jmp inloop
inok    clc
        rts
incancel
        sec
        rts

; ===================================================================================
; Copy a File to Another Drive (F5)
; ===================================================================================

copyfile
        lda count
        ora count+1
        bne +
cfno    jmp main
+       lda cur
        sta idx
        lda cur+1
        sta idx+1
        jsr entptr
        ldy #E_TYPE
        lda (ptr),y
        and #$7f
        beq cfno                ; DEL
        cmp #4                  ; only SEQ, PRG, USR
        bcs cfno
        sta ftype
        jsr askdrive            ; target drive in A
        bcs cfno
        sta target
        ldy #E_LEN              ; name
        lda (ptr),y
        sta namelen
        tax
        ldy #0
-       lda (ptr),y
        sta name,y
        iny
        dex
        bne -
        lda #<cftxt
        ldy #>cftxt
        jsr showmsg

        jsr nametype            ; source: "NAME,T,R" on channel 3
        lda #3
        ldy dev
        jsr openread
        bcc +
        jmp cferr
+
        jsr nametype            ; target: "NAME,T,W" on channel 4
        lda #"w"
        sta cmdbuf,x
        inx
        txa
        ldx #<cmdbuf
        ldy #>cmdbuf
        jsr SETNAM
        lda #4
        ldx target
        ldy #4
        jsr SETLFS
        jsr OPEN
        bcc +
        jmp cferr
+
        lda #0
        sta eof

cfloop  ldx #3                  ; fill the buffer from the source
        jsr CHKIN
        bcc +
        jmp cferr
+
        lda #<BUFFER
        sta bptr
        lda #>BUFFER
        sta bptr+1
-       jsr getbyte
        ldy #0
        sta (bptr),y
        inc bptr
        bne +
        inc bptr+1
+       lda eof
        bne +
        lda bptr+1
        cmp #>BUFEND
        bne -
+       jsr CLRCHN
        lda eof                 ; errors other than EOI?
        and #$bf
        bne cferr
        ldx #4                  ; write the buffer to the target
        jsr CHKOUT
        bcs cferr
        lda #<BUFFER
        sta ptr2
        lda #>BUFFER
        sta ptr2+1
-       lda ptr2
        cmp bptr
        bne +
        lda ptr2+1
        cmp bptr+1
        beq ++
+       ldy #0
        lda (ptr2),y
        jsr CHROUT
        inc ptr2
        bne -
        inc ptr2+1
        bne -
+       jsr CLRCHN
        jsr $ffe1               ; RUN/STOP cancels
        beq cfend
        lda eof
        beq cfloop

cfend   jsr CLRCHN              ; close both files
        lda #4
        jsr CLOSE
        lda #3
        jsr CLOSE
        lda dev                 ; status of the target drive
        pha
        lda target
        sta dev
        jsr status
        pla
        sta dev
        jmp main

cferr   jsr CLRCHN
        lda #4
        jsr CLOSE
        lda #3
        jsr CLOSE
        jsr status              ; error of the source drive
        jmp main

cftxt   .text "copying... (run/stop cancels)", 0

; Open "cmdbuf" + "R" for reading (X = length of cmdbuf) on drive Y,
; A = channel and secondary address; carry set on errors
openread
        sta tmp
        sty tmp2
        lda #"r"
        sta cmdbuf,x
        inx
        txa
        ldx #<cmdbuf
        ldy #>cmdbuf
        jsr SETNAM
        lda tmp
        ldx tmp2
        ldy tmp
        jsr SETLFS
        jmp OPEN

; cmdbuf = NAME,T, (X = length)
nametype
        ldx #0
-       lda name,x
        sta cmdbuf,x
        inx
        cpx namelen
        bcc -
        lda #","
        sta cmdbuf,x
        inx
        ldy ftype               ; S, P or U
        lda typeletter-1,y
        sta cmdbuf,x
        inx
        lda #","
        sta cmdbuf,x
        inx
        rts

typeletter .text "spu"

; Ask for the target drive at the status row: returns A = drive, carry set: cancel
askdrive
        lda #<asktxt
        ldy #>asktxt
        jsr showmsg
-       jsr GETIN
        beq -
        cmp #3
        beq ++
        cmp #"8"
        beq +
        cmp #"9"
        beq +
        cmp #"0"                ; 0..5 = 10..15
        bcc -
        cmp #"6"
        bcs -
        adc #10-"0"
        cmp dev
        beq -
        clc
        rts
+       sec
        sbc #"0"
        cmp dev                 ; not the same drive
        beq -
        clc
        rts
+       sec
        rts

asktxt  .text "target drive (8, 9, 0=10 .. 5=15)?", 0

; ===================================================================================
; Disk Image <-> Floppy Disk (F7)
; ===================================================================================
; Selected entry is a D64 image: write it to a floppy in another drive.
; Otherwise: read a floppy in another drive into a new image file NAME.D64.

diskcopy
        jsr askdrive            ; the floppy drive
        bcc +
        jmp main
+       sta target
        lda count
        ora count+1
        beq dcread
        lda cur
        sta idx
        lda cur+1
        sta idx+1
        jsr entptr
        ldy #E_TYPE
        lda (ptr),y
        bpl dcread
        ldy #E_LEN              ; image: only D64 (683 blocks or more)
        lda (ptr),y
        tax
        dex
        dex
        txa
        tay
        lda (ptr),y
        cmp #"6"
        bne dcread
        iny
        lda (ptr),y
        cmp #"4"
        bne dcread
        jmp dcwrite

; Read a floppy into a new image file
dcread  lda #22                 ; name of the new image (question above the input)
        sta msgrow
        lda #<dcnametxt
        ldy #>dcnametxt
        jsr showmsg
        lda #23
        sta msgrow
        jsr input
        bcc +
        jmp main
+       lda inlen
        beq dcread
        cmp #13
        bcs dcread
        ldx #0                  ; NAME.D64,P,W
-       lda inbuf,x
        sta cmdbuf,x
        inx
        cpx inlen
        bne -
        ldy #0
-       lda d64ext,y
        sta cmdbuf,x
        inx
        iny
        cpy #8
        bne -
        stx fnlen
        jsr dcopen              ; buffer "#" on the floppy drive (channel 2)
        bcc +
        jmp dcfail
+
        lda fnlen               ; image file on this drive (channel 3)
        ldx #<cmdbuf
        ldy #>cmdbuf
        jsr SETNAM
        lda #3
        ldx dev
        ldy #3
        jsr SETLFS
        jsr OPEN
        bcc +
        jmp dcfail
+
        lda #1
        sta track
dcrtrk  jsr dcprogress
        jsr sectors             ; sectors of the track in X
        stx nsec
        lda #0
        sta sector
-       jsr blockread           ; floppy sector into the buffer
        bcc +
        jmp dcfail
+
        inc sector
        lda sector
        cmp nsec
        bne -
        ldx #3                  ; write the track to the image
        jsr CHKOUT
        bcc +
        jmp dcfail
+
        jsr bufout
        jsr CLRCHN
        jsr $ffe1               ; RUN/STOP
        bne +
        jmp dcfail
+
        inc track
        lda track
        cmp #36
        bne dcrtrk
        jmp dcok

; Write a D64 image to a floppy
dcwrite ldy #E_LEN              ; NAME,P,R
        lda (ptr),y
        sta namelen
        tax
        ldy #0
-       lda (ptr),y
        sta name,y
        iny
        dex
        bne -
        lda #2
        sta ftype
        jsr dcopen
        bcs dcfail
        jsr nametype            ; image: "NAME,P,R" on channel 3
        lda #3
        ldy dev
        jsr openread
        bcs dcfail
        lda #1
        sta track
dcwtrk  jsr dcprogress
        jsr sectors
        stx nsec
        ldx #3                  ; read the track from the image
        jsr CHKIN
        bcs dcfail
        jsr bufin
        jsr CLRCHN
        lda #0
        sta sector
-       jsr blockwrite          ; buffer to the floppy sector
        bcs dcfail
        inc sector
        lda sector
        cmp nsec
        bne -
        jsr $ffe1
        beq dcfail
        inc track
        lda track
        cmp #36
        bne dcwtrk

dcok    jsr dcclose
        lda #<donetxt
        ldy #>donetxt
        jsr showmsg
        jsr waitkey
        jsr readdir
        jmp main
dcfail  jsr dcclose
        lda dev                 ; show the error of the floppy drive
        pha
        lda target
        sta dev
        jsr status
        pla
        sta dev
        jmp main

; Close all channels
dcclose jsr CLRCHN
        lda #3
        jsr CLOSE
        lda #2
        jsr CLOSE
        lda #15
        jmp CLOSE

; Open the buffer channel (2) and the command channel (15) of the floppy drive
dcopen  lda #1
        ldx #<hashtxt
        ldy #>hashtxt
        jsr SETNAM
        lda #2
        ldx target
        ldy #2
        jsr SETLFS
        jsr OPEN
        bcs +
        lda #0
        jsr SETNAM
        lda #15
        ldx target
        ldy #15
        jsr SETLFS
        jsr OPEN
+       rts

; Read sector "sector" of track "track" from the floppy into the buffer
blockread
        lda #"1"                ; "U1 2 0 T S"
        jsr blockcmd
        bcs +
        ldx #2
        jsr CHKIN
        bcs +
        jsr sectorptr
        ldy #0
-       jsr CHRIN
        sta (bptr),y
        iny
        bne -
        jsr CLRCHN
        clc
+       rts

; Write the buffer to sector "sector" of track "track" of the floppy
blockwrite
        ldx #15                 ; "B-P 2 0"
        jsr CHKOUT
        bcs +
        ldx #0
-       lda bptxt,x
        jsr CHROUT
        inx
        cpx #8
        bne -
        jsr CLRCHN
        ldx #2
        jsr CHKOUT
        bcs +
        jsr sectorptr
        ldy #0
-       lda (bptr),y
        jsr CHROUT
        iny
        bne -
        jsr CLRCHN
        lda #"2"                ; "U2 2 0 T S"
        jmp blockcmd
+       rts

bptxt   .text "b-p 2 0", 13

; Send "Un 2 0 T S" (n in A) and check the status
blockcmd
        sta utxt+1
        ldx #15
        jsr CHKOUT
        bcs ++
        ldx #0
-       lda utxt,x
        jsr CHROUT
        inx
        cpx #7
        bne -
        lda track
        jsr outnum
        lda #" "
        jsr CHROUT
        lda sector
        jsr outnum
        lda #13
        jsr CHROUT
        jsr CLRCHN
        ldx #15                 ; status: "00" is ok
        jsr CHKIN
        bcs ++
        jsr CHRIN
        sta tmp
        jsr CHRIN
        ora tmp
        sta tmp
-       jsr CHRIN               ; rest of the line
        cmp #13
        bne -
        jsr CLRCHN
        lda tmp
        cmp #"0"
        bne +
        clc
        rts
+       sec
+       rts

utxt    .text "u1 2 0 "

; Print the number in A (0..99) to the output channel
outnum  ldx #"0"-1
        sec
-       inx
        sbc #10
        bcs -
        adc #"0"+10
        pha
        txa
        cmp #"0"
        beq +
        jsr CHROUT
+       pla
        jmp CHROUT

; bptr = BUFFER + sector * 256
sectorptr
        lda #<BUFFER
        sta bptr
        lda sector
        clc
        adc #>BUFFER
        sta bptr+1
        rts

; Sectors of the track (1541 zones) in X
sectors ldx #21
        lda track
        cmp #18
        bcc +
        ldx #19
        cmp #25
        bcc +
        ldx #18
        cmp #31
        bcc +
        ldx #17
+       rts

; Write nsec * 256 bytes from the buffer to the output channel
bufout  lda #<BUFFER
        sta bptr
        lda #>BUFFER
        sta bptr+1
        ldx nsec
bopage  ldy #0
-       lda (bptr),y
        jsr CHROUT
        iny
        bne -
        inc bptr+1
        dex
        bne bopage
        rts

; Read nsec * 256 bytes from the input channel into the buffer
bufin   lda #<BUFFER
        sta bptr
        lda #>BUFFER
        sta bptr+1
        ldx nsec
bipage  ldy #0
-       jsr CHRIN
        sta (bptr),y
        iny
        bne -
        inc bptr+1
        dex
        bne bipage
        rts

; Show the track at the status row
dcprogress
        lda #<tracktxt
        ldy #>tracktxt
        jsr showmsg
        lda track
        sta num
        lda #0
        sta num+1
        lda #23
        jsr rowptr
        ldy #7
        jsr printnum
        rts

hashtxt .text "#"
d64ext  .text ".d64,p,w"
dcnametxt .text "name of the new d64 image (max. 12)", 0
tracktxt .text "track          (run/stop cancels)", 0
donetxt .text "done. press a key.", 0

; ===================================================================================
; Screen
; ===================================================================================

; Draw the frame: title, lines, help
frame   ldx #0                  ; clear the screen
-       lda #" "
        sta SCREEN,x
        sta SCREEN+$100,x
        sta SCREEN+$200,x
        sta SCREEN+$2e8,x
        lda #C_FRAME
        sta COLRAM,x
        sta COLRAM+$100,x
        sta COLRAM+$200,x
        sta COLRAM+$2e8,x
        inx
        bne -
        ldx #39                 ; title bar, line, help
-       lda titletxt,x
        ora #$80
        sta SCREEN,x
        lda #C_TITLE
        sta COLRAM,x
        lda #$40                ; horizontal line
        sta SCREEN+2*40,x
        lda helptxt,x
        sta SCREEN+24*40,x
        lda #C_HELP
        sta COLRAM+24*40,x
        dex
        bpl -
        ldx #helpcolend-helpcol-1   ; keys in the help line brighter
-       ldy helpcol,x
        lda #C_STAT
        sta COLRAM+24*40,y
        dex
        bpl -
        lda dev                 ; drive number in the title (columns 37, 38)
        ldx #"0"-1
        sec
-       inx
        sbc #10
        bcs -
        adc #"0"+10
        ora #$80
        sta SCREEN+38
        txa
        cmp #"0"
        bne +
        lda #" "
+       ora #$80
        sta SCREEN+37
        rts

titletxt .screen " sd2iec file browser 1.0       drive    "
helptxt  .screen "return open  _ back  f1 drive  f5 copy  "
helpcol  .byte 0, 1, 2, 3, 4, 5, 13, 21, 22, 31, 32
helpcolend

; Draw everything that changes: header, list, status
drawall lda #1                  ; disk name and id
        jsr rowptr
        ldy #39
        lda #" "
-       sta (scr),y
        dey
        bpl -
        ldy #0
-       lda diskname,y
        jsr petscr
        iny
        sta (scr),y
        lda #C_HEAD
        sta (col),y
        cpy #22
        bne -
        lda free                ; blocks free
        sta num
        lda free+1
        sta num+1
        ldy #25
        jsr printnum
        ldy #30
-       lda freetxt-30,y
        sta (scr),y
        iny
        cpy #35
        bne -
        ldy #39                 ; colours after the name
-       lda #C_FRAME
        sta (col),y
        dey
        cpy #22
        bne -
        lda #0                  ; the list
        sta row
-       jsr drawrow
        inc row
        lda row
        cmp #ROWS
        bne -
        lda #23                 ; status
        jsr rowptr
        ldy #0
-       lda stattxt,y
        beq +
        jsr petscr
        sta (scr),y
        lda #C_STAT
        sta (col),y
        iny
        cpy #40
        bne -
+       lda #" "
-       cpy #40
        beq +
        sta (scr),y
        iny
        bne -
+       lda count               ; "selected/count" at the right
        ora count+1
        beq +
        lda count
        sta num
        lda count+1
        sta num+1
        ldy #35
        jsr printnum
        lda #"/"
        sta SCREEN+23*40+34
        lda cur
        clc
        adc #1
        sta num
        lda cur+1
        adc #0
        sta num+1
        ldy #29
        jsr printnum
        ldx #11                 ; left aligned behind the "/"
-       lda SCREEN+23*40+35
        cmp #" "
        bne +
        lda SCREEN+23*40+36
        sta SCREEN+23*40+35
        lda SCREEN+23*40+37
        sta SCREEN+23*40+36
        lda SCREEN+23*40+38
        sta SCREEN+23*40+37
        lda SCREEN+23*40+39
        sta SCREEN+23*40+38
        lda #" "
        sta SCREEN+23*40+39
        dex
        bne -
+       ldy #29
        lda #C_HELP
-       sta (col),y
        iny
        cpy #40
        bne -
+       rts

freetxt .screen " free"

; Draw row "row" of the list
drawrow lda row
        clc
        adc #ROW0
        jsr rowptr
        ldy #39                 ; empty row
        lda #" "
-       sta (scr),y
        dey
        bpl -
        lda top                 ; idx = top + row
        clc
        adc row
        sta idx
        lda top+1
        adc #0
        sta idx+1
        lda idx                 ; beyond the end?
        cmp count
        lda idx+1
        sbc count+1
        bcc +
        rts
+       jsr entptr
        ldy #E_TYPE             ; colour by type
        ldx #C_IMAGE
        lda (ptr),y
        bmi ++
        ldx #C_DIR
        cmp #6
        beq ++
        ldx #C_PRG
        cmp #2
        beq ++
        ldx #C_OTHER
+
+       stx color
        cpx #C_DIR              ; blocks, right aligned in columns 1..5 (not for folders)
        beq +
        ldy #E_BLKS
        lda (ptr),y
        sta num
        iny
        lda (ptr),y
        sta num+1
        ldy #1
        jsr printnum
+
        ldy #0                  ; name in columns 8..23
-       sty tmp
        lda (ptr),y
        beq +
        jsr petscr
        pha
        tya
        clc
        adc #8
        tay
        pla
        sta (scr),y
        ldy tmp
        iny
        cpy #16
        bne -
+       ldy #E_TYPE             ; type in columns 26..28
        lda (ptr),y
        bpl +
        ldy #E_LEN              ; image: its extension
        lda (ptr),y
        tay
        dey
        dey
        dey
        ldx #0
-       lda (ptr),y
        and #$7f
        jsr petscr
        sta typetxt,x
        iny
        inx
        cpx #3
        bne -
        jmp ++
+       sta tmp                 ; type name
        asl
        adc tmp
        tax
        ldy #0
-       lda typenames,x
        jsr petscr
        sta typetxt,y
        inx
        iny
        cpy #3
        bne -
+       ldy #26
        ldx #0
-       lda typetxt,x
        sta (scr),y
        iny
        inx
        cpx #3
        bne -
        ldy #39                 ; colour, selection
        lda idx
        cmp cur
        bne +
        lda idx+1
        cmp cur+1
        bne +
-       lda (scr),y             ; selected: reverse bar
        ora #$80
        sta (scr),y
        lda #C_TITLE
        sta (col),y
        dey
        bpl -
        rts
+       lda #C_HELP             ; blocks and type grey, name in the colour of the type
-       sta (col),y
        dey
        cpy #24
        bne -
        lda color
-       sta (col),y
        dey
        cpy #6
        bne -
        lda #C_HELP
-       sta (col),y
        dey
        bpl -
        rts

; scr / col = start of screen row A
rowptr  tax
        lda rowlo,x
        sta scr
        sta col
        lda rowhi,x
        sta scr+1
        clc
        adc #(>COLRAM)-(>SCREEN)
        sta col+1
        rts

rowlo
        .for i = 0 to 24
        .byte <(SCREEN + i * 40)
        .endfor
rowhi
        .for i = 0 to 24
        .byte >(SCREEN + i * 40)
        .endfor

; Print num (0..65535) right aligned in 5 columns at (scr),y
printnum
        sty tmp2
        ldx #0                  ; digits
pndigit ldy #"0"
-       lda num
        sec
        sbc dectablo,x
        sta tmp
        lda num+1
        sbc dectabhi,x
        bcc +
        sta num+1
        lda tmp
        sta num
        iny
        bne -
+       tya
        sta digits,x
        inx
        cpx #5
        bne pndigit
        ldx #0                  ; leading zeros as spaces
-       lda digits,x
        cmp #"0"
        bne +
        lda #" "
        sta digits,x
        inx
        cpx #4
        bne -
+       ldx #0
        ldy tmp2
-       lda digits,x
        ora revflag
        sta (scr),y
        iny
        inx
        cpx #5
        bne -
        rts

dectablo .byte <10000, <1000, <100, <10, <1
dectabhi .byte >10000, >1000, >100, >10, >1

; PETSCII in A -> screen code in A
petscr  cmp #$20
        bcc pnul
        cmp #$40
        bcc ++
        cmp #$60
        bcc +
        cmp #$80
        bcc pminus20
        cmp #$a0
        bcc pnul
        cmp #$c0
        bcc pminus40
        cmp #$ff
        beq ppi
        and #$7f                ; $C0..$FE
        rts
+       and #$3f                ; $40..$5F
        rts
pminus40
        sec                     ; $A0..$BF
        sbc #$40
        rts
+       rts                     ; $20..$3F
pminus20
        sec
        sbc #$20
        rts
pnul    lda #" "
        rts
ppi     lda #$5e
        rts

; Show a message (A/Y = address of the text) at the status row
showmsg sta ptr2
        sty ptr2+1
        lda msgrow
        jsr rowptr
        ldy #0
-       lda (ptr2),y
        beq +
        jsr petscr
        sta (scr),y
        lda #1
        sta (col),y
        iny
        bne -
+       lda #" "
-       cpy #40
        beq +
        sta (scr),y
        iny
        bne -
+       rts

; "reading..." while the folder is read
showbusy
        lda #<busytxt
        ldy #>busytxt
        jmp showmsg

busytxt .text "reading...", 0

; ===================================================================================
; Input: Keyboard and Joystick
; ===================================================================================

; Wait for a key or a joystick movement, returns the key code in A and key
waitkey jsr GETIN
        bne wkkey
        lda $dc00               ; joystick port 2
        and #$1f
        eor #$1f
        beq wkrel               ; released
        cmp joylast
        beq wkrep
        sta joylast             ; new direction: act now
        lda $a2
        clc
        adc #18                 ; first repeat after 0.3 s
        sta joytime
        jmp wkjoy
wkrep   lda joylast             ; held: fire does not repeat
        and #$10
        bne waitkey
        lda $a2
        cmp joytime
        bne waitkey
        clc
        adc #4                  ; then 15 times per second
        sta joytime
wkjoy   lda joylast
        ldx #4                  ; bit -> key
-       lsr
        bcs +
        dex
        bpl -
        bmi waitkey
+       lda joykeys,x
        jmp wkkey
wkrel   sta joylast
        jmp waitkey
wkkey   sta key
        rts

joykeys .byte $0d, $1d, $9d, $11, $91     ; fire, right, left, down, up (bit 4..0)

; ===================================================================================
; Variables
; ===================================================================================

savesp  .byte 0                 ; stack pointer of BASIC
dev     .byte 8                 ; drive
target  .byte 9                 ; target drive of a copy
key     .byte 0                 ; last key
letter2 .byte 0                 ; letter to search
joylast .byte 0                 ; joystick state
joytime .byte 0                 ; time of the next repeat
depth   .byte 0                 ; folder depth (for the history)
row     .byte 0
msgrow  .byte 23                ; row of messages
revflag .byte 0
lineno  .byte 0                 ; line of the folder listing
free    .word 0                 ; blocks free
namelen .byte 0
typelen .byte 0
ftype   .byte 0
fnlen   .byte 0
track   .byte 0
sector  .byte 0
nsec    .byte 0
inlen   .byte 0
digits  .fill 5, 0
name    .fill 16, 0
typetxt .fill 5, 0
diskname .fill 22, 0
stattxt .fill 40, 0
inbuf   .fill 40, 0
cmdbuf  .fill 48, 0
histlo  .fill 16, 0             ; positions in the parent folders
histhi  .fill 16, 0
histtlo .fill 16, 0
histthi .fill 16, 0

        .cerror * > TABLE, "the program is too large"
