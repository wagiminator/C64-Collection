//   Magic Desk Compatible Cartridge Generator (c) 2013-2019  Žarko Živanov
//   Cartridge schematics and PCB design (c) 2013-2014 Marko Šolajić
//   E-mails: zzarko and msolajic at gmail

//   This program is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.

//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.

//   You should have received a copy of the GNU General Public License
//   along with this program.  If not, see <http://www.gnu.org/licenses/>.

//BasicUpstart2(init_menu)

// ZP usage:
// ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **
// 02 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F 20 21 22 23 24 25 26 27 28 29 2A FB FC FD FE
// ** ** ** ** ** **
// 52 57 58 59 5A 5B 5C 5D 5E 5F 60 61 62 63 64 65 66 67 68 69 6A 6B 6C 6D 6E

//--------------------------------
// RAM and ROM
//--------------------------------

.label SCREEN   = $0400
.label COLORR   = $D800 //$D800
.label CHROUT   = $FFD2
.label GETIN    = $FFE4
.label SCNKEY   = $FF9F
.label COLDRES  = $FCE2
.label BORDER   = $D020
.label PAPER    = $D021
.label IRQ_Vector = $0314
.label IRQ_Kernel = $EA31
.label CursorColor = $0286

.label drive    = $BA
.label status   = $90
.label LISTEN   = $FFB1
.label SECOND   = $FF93
.label UNLSTN   = $FFAE

//--------------------------------
// menu variables
//--------------------------------

// current menu variables
.label current_menu     = $25
.label prev_menu        = $5C
.label cmenu_offset     = $1A   // 2B offset from screen start
.label cmenu_offset_it  = $1C   // 2B offset from screen start for items
.label cmenu_offset_key = $26   // 2B offset from screen start for item key
.label cmenu_width      = $1E   // 1B menu inside width
.label cmenu_height     = $1F   // 1B menu inside height
.label cmenu_iwidth     = $20   // 1B menu inside width minus chars for key
.label cmenu_items_no   = $21   // 1B number of items
.label cmenu_max_first  = $22   // 1B max value for first item
.label cmenu_items      = $23   // 1B pointer to list of items
.label cmenu_first_idx  = $19   // 1B index of first item to show
.label cmenu_item_adr   = $14   // 2B pointer to current item text
.label cmenu_item_idx   = $16   // 1B current item index
.label cmenu_spacing    = $2A   // 1B menu items spacing (0-no spacing,1-1 line)

// misc menu variables
.label temp             = $02   // 1B temorary value
.label chrmem           = $FB   // 2B pointer to addres on screen
.label colmem           = $FD   // 2B pointer to addres in color ram
.label chrkeymem        = $28   // 2B pointer to addres on screen for key
.label drawm_line       = $17   // 1B current line when drawing items
.label drawm_idx        = $18   // 1B current item index when drawing items

//--------------------------------
// IRQ variables
//--------------------------------

.const SCR_FirstLine = 58
.const SCR_LastLine  = 242

// IRQ wave variables
.const WaveSpeed     = 2
.label CurrentLine   = $58
.label CurrentWave   = $59
.label FirstWave     = $5A
.label WavePause     = $5B

//--------------------------------
// cartridge start
//--------------------------------

* = $8000
//* = $0820
.word cold_start
.word init_menu
.byte $c3,$c2,$cd,$38,$30 // cartridge magic bytes, CBM80

cold_start:
        stx $d016
        jsr $FDA3       // IOINIT, init CIA,IRQ
        jsr $FD50       // RAMTAS, init memory
        jsr $FD15       // RESTOR, init I/O
        jsr $FF5B       // SCINIT, init video
        cli
        jsr $E453       // load BASIC vectors
        jsr $E3BF       // init BASIC RAM
        jsr $E422       // print BASIC start up messages
        ldx #$FB        // init BASIC stack
        txs

        // debug
        // lda #5
        // sta current_menu
        // lda #12
        // jmp prepare_run

        jmp init_menu

//--------------------------------
// menu drawing
//--------------------------------

scrolld:
        lda cmenu_first_idx
        cmp cmenu_max_first
        bpl !+
        inc cmenu_first_idx
!:      jmp dm1
scrollu:
        lda cmenu_first_idx
        beq !+
        dec cmenu_first_idx
!:      jmp dm1
paged:
        lda cmenu_first_idx
        clc
        adc cmenu_height
        sta cmenu_first_idx
        cmp cmenu_max_first
        bmi !+
        lda cmenu_max_first
        sta cmenu_first_idx
!:      jmp dm1
pageu:
        lda cmenu_first_idx
        sec
        sbc cmenu_height
        sta cmenu_first_idx
        cmp #0
        bpl !+
        lda #0
        sta cmenu_first_idx
!:      jmp dm1

init_menu:
        lda DoSound
        beq !+
        jsr sound_init
!:      jsr WaveVIC
        lda #$08  // disable c=+sh
        jsr CHROUT
        lda #$0e  // lower chars
        jsr CHROUT

        lda #0
        sta current_menu
        lda #$FF
        sta prev_menu

dm0:    lda current_menu
        cmp prev_menu
        beq dm2
        sta prev_menu
        lda #$60
        ldx #$A0
        ldy #6
        jsr clear_screen
        jsr setup_menu
        lda #0
        sta cmenu_first_idx
        //jsr draw_menu_border
dm1:    lda cmenu_first_idx
        sta drawm_idx
        //jsr draw_menu_items
dm2:    lda #$31 //jsr read_key
		jsr prgkey //none
        beq dm2
        cmp #$1D  // left-right key
        beq scrolld
        cmp #$11  // up-down key
        beq scrollu
        cmp #$9D  // shift+left-right key
        beq paged
        cmp #$91  // shift+up-down key
        beq pageu
        cmp #$0D  // return
        beq nextmenu
        cmp #$5F  // left arrow
        beq basic
        cmp #$85  // F1
        bmi prgkey
        cmp #$8D  // 89 - F7+1, 8D - F8+1
        bpl prgkey
        sec
        sbc #$85
        tay
        lda menu_items_no,y
        beq dm2
        sty current_menu
        jmp dm0

prgkey: ldy #0  // check if some program is selected
!:      cmp menu_keys,y
        beq !+
        iny
        cpy #menu_keys_no
        bne !-
        jmp dm2
!:      iny  // last item is basic, do not check that key
        tya
        ldy current_menu
        cmp menu_items_no,y
        bpl dm2
!:      tay
        dey
        tya
        jmp prepare_run
basic:
        ldy cmenu_items_no
        dey
        tya
        jmp prepare_run
nextmenu:
        ldy current_menu
        sty temp
        iny
        cpy #8
        bne !+
        ldy #0
!:      lda menu_items_no,y
        sty current_menu
        bne !+
        lda #0
        sta current_menu
!:      jmp dm0

draw_menu_border:
        lda cmenu_offset
        sta chrmem
        sta colmem
        lda #>SCREEN
        clc
        adc cmenu_offset+1
        sta chrmem+1
        lda #>COLORR
        clc
        adc cmenu_offset+1
        sta colmem+1

        ldx #0
        jsr draw_menu_border_line
        jsr menu_next_line

        lda cmenu_height
        ldy cmenu_spacing
        beq dmb1
        asl
        tay
        dey
        tya
dmb1:   pha
        ldx #4
        jsr draw_menu_border_line
        lda cmenu_spacing
        beq !+
        pla
        pha
        and #1
        beq dmb2
!:      ldy #1
        ldx #0
!:      lda menu_key_chr,x
        sta (chrmem),y
        lda menu_key_col,x
        sta (colmem),y
        iny
        inx
        cpx #3
        bmi !-
dmb2:   jsr menu_next_line
        pla
        tay
        dey
        tya
        bne dmb1

        ldx #8
        jsr draw_menu_border_line
        jsr menu_next_line
        ldx #12
        jsr draw_menu_border_line
        rts

draw_menu_border_line:
        ldy #0
        lda menu_border_chr,x
        sta (chrmem),y
        lda menu_border_col,x
        sta (colmem),y
        iny
        lda cmenu_width
        sta temp
        inc temp
        inx
!:      lda menu_border_chr,x
        sta (chrmem),y
        lda menu_border_col,x
        sta (colmem),y
        iny
        cpy temp
        bmi !-
        inx
        lda menu_border_chr,x
        sta (chrmem),y
        lda menu_border_col,x
        sta (colmem),y
        inx
        iny
        lda menu_border_chr,x
        sta (chrmem),y
        lda menu_border_col,x
        sta (colmem),y
        rts

menu_next_line:
        clc
        lda chrmem
        adc #40
        sta chrmem
        sta colmem
        bcc !+
        inc chrmem+1
        inc colmem+1
!:      clc
        lda chrkeymem
        adc #40
        sta chrkeymem
        bcc !+
        inc chrkeymem+1
!:      rts

draw_menu_items:
        lda cmenu_offset_it
        sta chrmem
        sta colmem
        lda #>SCREEN
        clc
        adc cmenu_offset_it+1
        sta chrmem+1
        lda #>COLORR
        clc
        adc cmenu_offset_it+1
        sta colmem+1

        lda cmenu_offset_key
        sta chrkeymem
        lda #>SCREEN
        clc
        adc cmenu_offset_key+1
        sta chrkeymem+1

        lda drawm_idx
        jsr find_menu_item
        lda #0
        sta drawm_line
!:      jsr draw_menu_item_line
        jsr menu_next_line
        jsr find_next_menu_item
        ldy cmenu_spacing
        beq dmi1
        jsr menu_next_line
dmi1:   inc drawm_idx
        inc drawm_line
        lda drawm_line
        cmp cmenu_height
        bne !-
        rts

draw_menu_item_line:
        ldy #0
!:      lda (cmenu_item_adr),y
        beq !+
        sta (chrmem),y
        iny
        cpy cmenu_iwidth
        bne !-
!:      lda menu_chr_back
        cpy cmenu_iwidth
        beq !+
        sta (chrmem),y
        iny
        bne !-
!:      ldy drawm_idx   // convert PETSCII to inverted SCREEN codes
        iny             // last item will always be "Basic", and its key is left arrow
        cpy cmenu_items_no
        bne !+
        ldy #menu_keys_no+1
!:      dey
        lda menu_keys,y
        cmp #$C1        // codes C1-DA translate to C1-DA
        bcs dmil1
        cmp #$40
        bcc !+
        clc             // codes 41-5A translate to 81-9A
        adc #$40
        .byte $2C       // bit instruction
!:      ora #$80        // codes 41-5A translate to B0-B9
dmil1:  ldy #0
        sta (chrkeymem),y
        rts

//--------------------------------
// item list functions
//--------------------------------

// find menu item whose index is in A register and set menu item pointer
find_menu_item:
        sta cmenu_item_idx
        lda cmenu_items
        sta cmenu_item_adr
        lda cmenu_items+1
        sta cmenu_item_adr+1
        ldx #0
fmi1:   cpx cmenu_item_idx
        beq fmi2
        ldy #0
!:      lda (cmenu_item_adr),y
        beq !+
        iny
        bne !-
!:      iny
        tya
        clc
        adc cmenu_item_adr
        bcc !+
        inc cmenu_item_adr+1
!:      sta cmenu_item_adr
        inx
        jmp fmi1
fmi2:   rts

find_next_menu_item:
        ldx cmenu_item_idx
        inc cmenu_item_idx
        jmp fmi1

//--------------------------------
// menu setup
//--------------------------------

// setup all current menu variables for menu whose index is in current_menu
setup_menu:
        ldx current_menu
        ldy menu_items_no,x
        sty cmenu_items_no
        tya
        sec
        sbc menu_height,x
        sta cmenu_max_first
        ldy menu_width,x
        sty cmenu_width
        dey
        dey
        dey
        sty cmenu_iwidth        // menu_width-3 (width of key display)
        ldy menu_height,x
        sty cmenu_height
        ldy menu_spacing,x
        sty cmenu_spacing
        txa                     // offsets are 2-byte
        asl
        tax
        lda menu_offset,x
        sta cmenu_offset
        lda menu_offset+1,x
        sta cmenu_offset+1
        lda menu_items,x
        sta cmenu_items
        lda menu_items+1,x
        sta cmenu_items+1
        // calculate offset for menu items
        lda cmenu_offset+1
        sta cmenu_offset_it+1
        lda cmenu_offset
        clc
        adc #44                 // 1 line + 3 chars for key display
        sta cmenu_offset_it
        bcc !+
        inc cmenu_offset_it+1
        // calculate offset for menu item keys
!:      lda cmenu_offset+1
        sta cmenu_offset_key+1
        lda cmenu_offset
        clc
        adc #42                 // 1 line + 2
        sta cmenu_offset_key
        bcc !+
        inc cmenu_offset_key+1

!:      lda menu_names,x
        sta chrmem
        lda menu_names+1,x
        sta chrmem+1
        ldy #0
!:      lda (chrmem),y
        ora #$80
        sta SCREEN,y
        lda menu_help,y
        ora #$80
        //sta SCREEN+24*40,y
        iny
        cpy #40
        bne !-
        rts

//--------------------------------
// utility functions
//--------------------------------

// A - border/paper, X - char, Y - color
clear_screen:
        sta PAPER
        lsr
        lsr
        lsr
        lsr
        sta BORDER
        txa
        ldx #0
!:      sta SCREEN,x
        sta SCREEN+$100,x
        sta SCREEN+$200,x
        sta SCREEN+$2E8,x
        pha
        tya
        sta COLORR,x
        sta COLORR+$100,x
        sta COLORR+$200,x
        sta COLORR+$2E8,x
        pla
        inx
        bne !-
        rts

// returns key as read by GETIN, makes a small pause before reading
read_key:
        ldy #40
        ldx #0
!:      dex
        bne !-
        dey
        bne !-
!:      jsr SCNKEY
        jsr GETIN
        beq !-
        rts

// find first active drive
FindFirstDrive:
        lda #8
        sta drive
!:      ldx #0
        stx status
        jsr LISTEN
        lda #$FF
        jsr SECOND
        lda status
        bpl !+
        jsr UNLSTN
        inc drive
        lda drive
        cmp #31
        bne !-
        lda #8          // if not found, set 08
        sta drive
!:      jsr UNLSTN
        rts

//--------------------------------
// IRQ
//--------------------------------

// setup VIC II for waving menu
WaveVIC:
        sei
        lda #<WaveIRQ
        sta IRQ_Vector
        lda #>WaveIRQ
        sta IRQ_Vector+1
        lda #$7f        //disable cia irq
        sta $dc0d
        sta $dd0d
        lda $dc0d
        lda $dd0d
        lda $d01a       //enable raster irq
        ora #01
        sta $d01a
        lda $d011       //raster irq bit 8
        and #$7f
        sta $d011
        lda #SCR_FirstLine
        sta $d012
        sta CurrentLine
        lda #06
        sta $d020
        lda #00
        sta $d021
        sta CurrentWave
        sta FirstWave
        sta WavePause
/*
        lda $dd00 //vic bank
        and #$fc
        ora #00   !0-c000,1-8000
        sta $dd00
        //vic setup $d018
        //bit 7-4 screen  0001-$0400
        //bit 3-1 charmem  001-$0800
        lda #%00010100
        sta $d018
*/
        cli
        rts

// return VIC II to normal state
NormalVIC:
        sei
        lda #<IRQ_Kernel
        sta IRQ_Vector
        lda #>IRQ_Kernel
        sta IRQ_Vector+1
        jsr $FDA3      // IOINIT, init CIA,IRQ
        //jsr $FD50      // RAMTAS, init memory
        jsr $FD15      // RESTOR, init I/O
        jsr $FF5B      // SCINIT, init video
        //lda $d01a
        //and #$fe
        //sta $d01a
        //lda #$c8
        //sta $d016
        lda RunBorderColor
        sta $d020
        lda RunPaperColor
        sta $d021
        lda RunInkColor
        sta CursorColor
        ldx #$20  // clear SID registers
        lda #$00
!:      sta $D400,x
        dex 
        bpl !-
        cli
        rts

// IRQ for menu waves
WaveIRQ:
        lda DoWave
        beq dosound
        nop;nop;nop;nop;nop;nop
        ldy CurrentWave
        lda WaveTable,y
        sta $d016
        iny
        cpy #WaveTableMax
        bne irq0
        ldy #00
irq0:   sty CurrentWave
        ldy CurrentLine
        cpy #SCR_LastLine-8
        bcc !+
        dey
!:      tya
        clc
        adc #08
        cmp #SCR_LastLine
        bcc irq1
        lda #SCR_FirstLine
irq1:   sta CurrentLine
        sta $d012
        cmp #SCR_FirstLine
        bne irqend
        lda #$c8
        sta $d016
        lda FirstWave
        sta CurrentWave
        ldy WavePause
        iny
        cpy #WaveSpeed
        bne irq3
        ldy #00
        ldx FirstWave
        inx
        cpx #WaveTableMax
        bne irq4
        ldx #00
irq4:   stx FirstWave
irq3:   sty WavePause

dosound:
        ldx #$50
!:      lda $D41B    //Oscillator 3 Output
        sta $D401    //Voice 1: Frequency Control - High-Byte
        dex 
        bne !-

irqend: ldy $d019
        sty $d019
        pla;tay;pla;tax;pla
        rti

WaveTable: .byte $c0,$c0,$c1,$c1,$c2,$c3,$c4,$c5,$c5,$c6,$c6,$c7,$c7
           .byte $c7,$c7,$c6,$c6,$c5,$c5,$c4,$c3,$c2,$c1,$c1,$c0,$c0
.label WaveTableMax = *-WaveTable

//--------------------------------
// memory copy
//--------------------------------

.label TableAddress  = $FB
.label ProgramIndex  = $FD

prepare_run:
        sta ProgramIndex
        ldy #0
!:      cpy current_menu
        beq !+
        ldx menu_items_no,y
        txa
        clc
        adc ProgramIndex
        sta ProgramIndex
        iny
        bne !-
!:      jsr NormalVIC
        jsr $E453       // load BASIC vectors
        jsr $E3BF       // init BASIC RAM
        //jsr $A68E       // set current character pointer to start of basic - 1
        //jsr $E422       // print BASIC start up messages
        //ldx #$FB
        //txs                     
        jsr FindFirstDrive

startCopy:
        ldy #CartCopyLen    // copy routine to 0340
!:      lda CartCopy0340,y
        sta $0340,y
        dey
        bpl !-
        ldy #0              //calculate program table element address
        sty TableAddress+1  //each table element is 9 bytes
        lda ProgramIndex    //ProgramIndex*8
        asl 
        rol TableAddress+1
        asl 
        rol TableAddress+1
        asl 
        rol TableAddress+1
        sta TableAddress
        lda ProgramIndex    //ProgramIndex*8 + ProgramIndex
        clc
        adc TableAddress
        sta TableAddress
        bcc sc2
        inc TableAddress+1
sc2:    lda ProgramTable    //add Program table address
        clc
        adc TableAddress
        sta TableAddress
        lda ProgramTable+1
        adc TableAddress+1
        sta TableAddress+1
        lda (TableAddress),y    //set values in copy routine
        sta CartBank
        iny;lda (TableAddress),y
        sta CartAddr
        iny;lda (TableAddress),y
        sta CartAddr+1
        iny;lda (TableAddress),y
        sta PrgLenLo
        iny;lda (TableAddress),y
        sta PrgLenHi
        iny;lda (TableAddress),y
        sta MemAddr
        iny;lda (TableAddress),y
        sta MemAddr+1
        iny;lda (TableAddress),y
        sta pstart+1
        iny;lda (TableAddress),y
        sta pstart+2
        beq sc3     //if high byte of start address != 0
        lda #$4c    //change lda to jmp
        sta pstart  //else, run as basic
sc3:    jmp $0340

CartCopy0340:
.pseudopc $0340 {
        ldx PrgLenLo:#00    //program length lo
        ldy PrgLenHi:#00    //program length hi
cbank:  lda CartBank:#00    //cartridge start bank
        sta $de00           //cartridge bank switching address
caddr:  lda CartAddr:$8200  //cartridge start adr
        sta MemAddr:$0801   //c64 memory start adr
        dex
        cpx #$ff
        bne cc1
        dey
        cpy #$ff
        beq crtoff
cc1:    inc MemAddr
        bne cc2
        inc MemAddr+1
cc2:    inc CartAddr
        bne caddr
        inc CartAddr+1
        lda CartAddr+1
        cmp #$a0        //next bank?
        bne caddr
        inc CartBank
        lda #$80        //cartridge bank is on $8000-$9fff
        sta CartAddr+1
        jmp cbank
crtoff: lda #$ff        //turn off cartridge
        sta $de00
        lda MemAddr     //set end of program (var start)
        sta $2d
        sta $2f
        sta $31
        sta $ae
        lda MemAddr+1
        sta $2e
        sta $30
        sta $32
        sta $af
pstart: lda $0801       //start the program
        lda #00         // basic start
        jsr $A871       // clr
        jsr $A533       // re-link
        jsr $A68E       // set current character pointer to start of basic - 1
        jmp $A7AE       // run
}
.label CartCopyLen = *-CartCopy0340

//--------------------------------
// menu sound
//--------------------------------

sound_init:
        lda #$00
        sta $D40F    //Voice 3: Frequency Control - High-Byte
        lda #$1E
        sta $D40E    //Voice 3: Frequency Control - Low-Byte
        lda #$F0
        sta $D414    //Voice 3: Sustain / Release Cycle Control
        sta $D406    //Voice 1: Sustain / Release Cycle Control
        lda #$81
        sta $D412    //Voice 3: Control Register
        lda #$11
        sta $D404    //Voice 1: Control Register
        lda #$83
        sta $D418    //Select Filter Mode and Volume
        rts

//--------------------------------
// menu data
//--------------------------------

.encoding "screencode_mixed"

// menu chars corners
.const MENU_CHR_UL   = $EC
.const MENU_CHR_UR   = $FB
.const MENU_CHR_DL   = $FC
.const MENU_CHR_DR   = $FE
.const MENU_COL_UL   = 6 //10
.const MENU_COL_UR   = 6 //10
.const MENU_COL_DL   = 6 //10
.const MENU_COL_DR   = 6 //10

// menu chars sides
.const MENU_CHR_L       = $61
.const MENU_CHR_R       = $E1
.const MENU_CHR_U       = $E2
.const MENU_CHR_D       = $62
.const MENU_COL_L       = 6 //10
.const MENU_COL_R       = 6 //10
.const MENU_COL_U       = 6 //10
.const MENU_COL_D       = 6 //10

// menu chars misc
.const MENU_CHR_BACK    = $20
.const MENU_CHR_SHAD    = $20 // $E6
.const MENU_COL_BACK    = 6 //5
.const MENU_COL_SHAD    = 6 //2
.const MENU_COL1_KEY    = 6 //12
.const MENU_COL2_KEY    = 6  //12
.const MENU_COL_NAME    = 6 //4
.const SCREEN_CHR       = $20 //$A0 
.const SCREEN_COL       = 6 //6

// menu border first, middle, bottom and last line chars
menu_border_chr:
        .byte MENU_CHR_UL,MENU_CHR_U,MENU_CHR_UR,SCREEN_CHR
        .byte MENU_CHR_L,MENU_CHR_BACK,MENU_CHR_R,MENU_CHR_SHAD
        .byte MENU_CHR_DL,MENU_CHR_D,MENU_CHR_DR,MENU_CHR_SHAD
        .byte SCREEN_CHR,MENU_CHR_SHAD,MENU_CHR_SHAD,MENU_CHR_SHAD

// menu border first, middle, bottom and last line colors
menu_border_col:
        .byte MENU_COL_UL,MENU_COL_U,MENU_COL_UR,SCREEN_COL
        .byte MENU_COL_L,MENU_COL_BACK,MENU_COL_R,MENU_COL_SHAD
        .byte MENU_COL_DL,MENU_COL_D,MENU_COL_DR,MENU_COL_SHAD
        .byte SCREEN_COL,MENU_COL_SHAD,MENU_COL_SHAD,MENU_COL_SHAD

// misc menu chars
menu_chr_back:  .byte MENU_CHR_BACK
menu_key_chr:   .byte $F5,$B1,$F6
menu_key_col:   .byte MENU_COL1_KEY,MENU_COL2_KEY,MENU_COL1_KEY

// keys for selecting a program
menu_keys:      .fill 9,i+$31   // 1-9
                .byte $30       // 0
                .fill 26,i+$41  // a-z
                .fill 26,i+$C1  // A-Z
.label menu_keys_no = *-menu_keys
                .byte $5F       // left arrow

*=* "Menu Data" virtual

ProgramTable:   .word programs

// program run colors
RunBorderColor: .byte 12 //14
RunPaperColor:  .byte 6 //6 15
RunInkColor:    .byte 0  //14
DoWave:         .byte 0  //menu waving
DoSound:        .byte 0  //menu sound

// menu data for 8 menus
menu_items_no:  .byte 1,0,0,0,0,0,0,0
menu_offset:    .word 85,0,0,0,0,0,0,0
menu_width:     .byte 15,0,0,0,0,0,0,0
menu_height:    .byte 10,0,0,0,0,0,0,0
menu_spacing:   .byte 1,0,0,0,0,0,0,0

menu_names:     .word menu_name1,0,0,0,0,0,0,0
menu_items:     .word menu_items1,0,0,0,0,0,0,0

menu_help:      .text "(Shift)CRSR: Scroll, Fn/Ret: Menu select"

menu_name1:     .text "            Testing...                  "

menu_items1:
        .text @"Testing1\$00"
        .text @"Testing2\$00"

programs:
// program table
// .byte bank
// .word address in bank
// .word length
// .word load address
// .word start address
//     if hi byte=0, then run as basic

// ne radi
// f4/6 64tester
// f4/c basic 64 compiler
// f5/e turbocopy 5
// f5/h turbo nibbler 5

