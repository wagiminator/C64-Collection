// ===================================================================================
// Low Level Fast Loader Transfer Routines (Assembler)                        * v1.0 *
// ===================================================================================
//
// Functions available:
// --------------------
// IEC_ATN_VECT                     Bus interrupt (ATN acknowledge, DreamLoad commands)
// jiffy_receive(&busstate)         JiffyDOS: receive a byte
// jiffy_send(b, eoi, loadflags)    JiffyDOS: send a byte, returns non-zero on ATN
// turbodisk_byte(b)                Turbodisk: send a byte
// turbodisk_buffer(p, n)           Turbodisk: send n bytes
// clk_data_handshake()             wait for a CLOCK/DATA handshake (FC3, ULoad3, ELoad)
// fastloader_fc3_send_block(p)     Final Cartridge III: send a 256 byte block
// fc3_get_byte()                   Final Cartridge III: receive a byte (SAVE)
// fc3_oldfreeze_pal/ntsc_send(b)   Final Cartridge III: freezer files (old loader)
// dreamload_get_byte()             DreamLoad: receive a byte
// dreamload_send_byte(b)           DreamLoad: send a byte
// uload3_get_byte()                ULoad Model 3: receive a byte (-1 on ATN)
// uload3_send_byte(b)              ULoad Model 3: send a byte
// epyxcart_send_byte(b)            Epyx FastLoad cartridge: send a byte
// geos_get/send_byte_*()           GEOS: transfer routines (1/2 MHz, 1581)
// wheels_*()                       Wheels: transfer routines
// ar6_1581_send_byte(b)            Action Replay 6 (1581 mode): send a byte
// ar6_1581p_get_byte()             Action Replay 6 (1581 mode): receive a byte
// n0sdos_send_byte(b)              N0stalgia loaders: send a byte
//
// Notes:
// ------
// - The routines are cycle counted for F_CPU = 8 MHz.
// - The IEC lines are driven through the data direction register: a set bit in IEC_OUT
//   pulls the line low, a cleared bit releases it.
// - All IEC lines must be on the same port (see config.h).
// - The timing values were determined by Ingo Korb and the sd2iec contributors with
//   real hardware; they are taken over unchanged.
//
// Based on sd2iec by Ingo Korb (https://www.sd2iec.de), GPL v2 only.
//   Copyright (C) 2007-2017  Ingo Korb <ingo@akana.de>
//   Copyright (C) 2007-2008  M.Kiesel <mayne@users.sourceforge.net>
//   Final Cartridge III, DreamLoad fastloader support:
//   Copyright (C) 2008  Thomas Giesel <skoe@directbox.com>
// JiffyDOS protocol documentation by Jochen Adler (http://www.nlq.de).
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#include <avr/io.h>
#include "gpio.h"                              // pin names PA0..PD7 as 0..31
#include "../config.h"
#include "fastloader.h"

#if F_CPU != 8000000
  #error "The fast loader routines are cycle counted for 8 MHz"
#endif

// ===================================================================================
// IEC Port Definitions
// ===================================================================================
#if   (PIN_IEC_ATN >> 3) == 0
  #define IEC_IN          _SFR_IO_ADDR(PINA)
  #define IEC_OUT         _SFR_IO_ADDR(DDRA)
  #define IEC_PCMSK_ADDR  _SFR_MEM_ADDR(PCMSK0)
  #define IEC_ATN_VECT    PCINT0_vect
#elif (PIN_IEC_ATN >> 3) == 1
  #define IEC_IN          _SFR_IO_ADDR(PINB)
  #define IEC_OUT         _SFR_IO_ADDR(DDRB)
  #define IEC_PCMSK_ADDR  _SFR_MEM_ADDR(PCMSK1)
  #define IEC_ATN_VECT    PCINT1_vect
#elif (PIN_IEC_ATN >> 3) == 2
  #define IEC_IN          _SFR_IO_ADDR(PINC)
  #define IEC_OUT         _SFR_IO_ADDR(DDRC)
  #define IEC_PCMSK_ADDR  _SFR_MEM_ADDR(PCMSK2)
  #define IEC_ATN_VECT    PCINT2_vect
#else
  #define IEC_IN          _SFR_IO_ADDR(PIND)
  #define IEC_OUT         _SFR_IO_ADDR(DDRD)
  #define IEC_PCMSK_ADDR  _SFR_MEM_ADDR(PCMSK3)
  #define IEC_ATN_VECT    PCINT3_vect
#endif

#define IEC_PIN_ATN       (PIN_IEC_ATN   & 7)   // bit numbers within the port
#define IEC_PIN_DATA      (PIN_IEC_DATA  & 7)
#define IEC_PIN_CLOCK     (PIN_IEC_CLOCK & 7)
#define IEC_BIT_ATN       (1 << IEC_PIN_ATN)    // bit masks
#define IEC_BIT_DATA      (1 << IEC_PIN_DATA)
#define IEC_BIT_CLOCK     (1 << IEC_PIN_CLOCK)

// Timing offsets for JiffyDOS read/write. These values are added to the delay loop
// counter before the first bit pair read/write. They were determined experimentally
// to center the range of working OSCCAL values around the default value.
#define JIFFY_OFFSET_WRITE 30
#define JIFFY_OFFSET_READ  30

// ===================================================================================
        .section .text

        ;; ===================================================================
        ;;  Pin change interrupt of the IEC port (ATN, and CLOCK for DreamLoad)
        ;; ===================================================================
        ;
        ; ATN low:   acknowledge by pulling DATA (like the ATNA logic of the 1541),
        ;            old DreamLoad: receive a command (track/sector)
        ; CLOCK low: DreamLoad: receive a command (track/sector)
        ;
        ; Functions called from here may only change r0, r18, r19, r24.
        ;
        .global IEC_ATN_VECT
IEC_ATN_VECT:
        push    r18
        in      r18, _SFR_IO_ADDR(SREG)
        push    r18
        push    r19
        push    r24
        push    r0

        lds     r18, IEC_PCMSK_ADDR
        bst     r18, IEC_PIN_ATN        ; ATN interrupt enabled?
        brtc    iiv_atn_irq_end         ; no -> skip
        sbic    IEC_IN, IEC_PIN_ATN     ; ATN low?
        rjmp    iiv_atn_irq_end         ; no -> skip

#if FL_DREAMLOAD
        lds     r18, detected_loader
        cpi     r18, FLCODE_DREAMLOAD_OLD
        brne    iiv_atn_ack
        rcall   dreamload_get_command_old ; old DreamLoad: receive a command
        rjmp    iiv_atn_irq_end
iiv_atn_ack:
#endif
        sbi     IEC_OUT, IEC_PIN_DATA   ; ATN acknowledge: DATA low

iiv_atn_irq_end:
#if FL_DREAMLOAD
        lds     r18, IEC_PCMSK_ADDR
        bst     r18, IEC_PIN_CLOCK      ; CLOCK interrupt enabled?
        brtc    iiv_irq_end             ; no -> skip
        sbic    IEC_IN, IEC_PIN_CLOCK   ; CLOCK low?
        rjmp    iiv_irq_end             ; no -> skip
        lds     r18, detected_loader
        cpi     r18, FLCODE_DREAMLOAD
        brne    iiv_irq_end
        rcall   dreamload_get_command   ; DreamLoad: receive a command
iiv_irq_end:
#endif

        pop     r0
        pop     r24
        pop     r19
        pop     r18
        out     _SFR_IO_ADDR(SREG), r18
        pop     r18
        reti


        ;; ===================================================================
        ;;  Utility routines
        ;; ===================================================================

        ;;  Used by the macro below, don't call directly
cycleloop2:
        nop                     ; 1
cycleloop1:
        nop                     ; 1
cycleloop0:
        dec     r18             ; 1
        brne    cycleloop0      ; 2/1
        ret                     ; 4

        ;; This macro waits for the specified number of cycles
        ;;  Uses r18
        .macro delay_cycles num
        ldi     r18, 1 + (\num - 10) / 3 ; 1
        .if (\num - 10) % 3 == 0
        rcall   cycleloop0      ; 3
        .elseif (\num - 10) % 3 == 1
        rcall   cycleloop1      ; 3
        .else
        rcall   cycleloop2      ; 3
        .endif
        .endm


        ;; Including the rcall this will delay for 8 cycles
delay1us:
        nop                     ; 1
        ret                     ; 4

        ;; wait for ATN high
wait_atn_high:
        sbis    IEC_IN, IEC_PIN_ATN
        rjmp    wait_atn_high
        ret

        ;; wait for ATN low
wait_atn_low:
        sbic    IEC_IN, IEC_PIN_ATN
        rjmp    wait_atn_low
        ret


        ;; ====================================================================
        ;;  JiffyDOS
        ;; ====================================================================

        ;;
        ;; Receives a single byte using the JiffyDOS protocol
        ;; return uint8_t reveicedbyte (r24)
        ;;
        .global jiffy_receive
jiffy_receive:
        ;; Disable interrupts
        cli

        ;; Move pointer to EOF variable into Z
        movw    r30,r24
        clr     r24             ; clear output register
        clr     r25             ; clear high byte of return value

        ;; Set clock+data high
        cbi     IEC_OUT, IEC_PIN_CLOCK
        cbi     IEC_OUT, IEC_PIN_DATA

        ;; Wait until clock is high and emulate ATN-Ack
0:      sbis    IEC_IN, IEC_PIN_ATN
        sbi     IEC_OUT, IEC_PIN_DATA ; Data low if ATN is low
        sbis    IEC_IN, IEC_PIN_CLOCK
        rjmp    0b

        ;; Wait for 13us from clock high -> 104 cycles
        delay_cycles 104 + JIFFY_OFFSET_READ ; 104 + Offset

        ;; Read bit 5+4
        in      r0, IEC_IN ; 1

        ;; Store bits in r24
        bst     r0, IEC_PIN_DATA  ; 1 - get data bit
        bld     r24, 5            ; 1 - store as bit 5
        bst     r0, IEC_PIN_CLOCK ; 1 - get clock bit
        bld     r24, 4            ; 1 - store as bit 4

        ;; Wait 13us -> 104-5=99 cycles
        delay_cycles 99         ; 99

        ;; Read bit 7+6
        in      r0, IEC_IN ; 1

        ;; Store bits in r24
        bst     r0, IEC_PIN_DATA  ; 1 - get data bit
        bld     r24, 7            ; 1 - store as bit 7
        bst     r0, IEC_PIN_CLOCK ; 1 - get clock bit
        bld     r24, 6            ; 1 - store as bit 6

        ;; Wait 11us -> 88-5=83 cycles
        delay_cycles 83         ; 83

        ;; Read bit 1+3 [sic]
        in      r0, IEC_IN ; 1
        bst     r0, IEC_PIN_DATA  ; 1 - get data bit
        bld     r24, 1            ; 1 - store as bit 1
        bst     r0, IEC_PIN_CLOCK ; 1 - get clock bit
        bld     r24, 3            ; 1 - store as bit 3

        ;; Wait 13us -> 104-5=99 cycles
        delay_cycles 99         ; 99

        ;; Read Bit 0+2
        in      r0, IEC_IN ; 1
        bst     r0, IEC_PIN_DATA  ; 1 - get data bit
        bld     r24, 0            ; 1 - store as bit 0
        bst     r0, IEC_PIN_CLOCK ; 1 - get clock bit
        bld     r24, 2            ; 1 - store as bit 2

        ;; Wait 13us -> 104-5=99 cycles
        delay_cycles 99         ; 99

        ;; Read EOI mark
        in      r19, IEC_IN ; 1

        ;; Wait 6us -> 48-1=47 cycles
        delay_cycles 47         ; 47

        ;; Data low
        sbi     IEC_OUT, IEC_PIN_DATA ; 1
        delay_cycles 10*8       ; make sure the C64 can see that

        ;; Post-process data and store last bus state
        com     r24             ; invert received data (result)
        st      Z, r19          ; store last bus state
        sei                     ; enable interrupts
        ret



        ;; Shifts the lowest two bits from r0 to r19
        ;; and sends then on the bus at cycle 12
        ;; Needs 16 cycles including the rcall
jiffy_sendbits:
        in      r19, IEC_OUT ; 1 - read port
        andi    r19, ~(IEC_BIT_DATA|IEC_BIT_CLOCK|IEC_BIT_ATN)
                                    ; 1 - mask IEC bits
        bst     r0, 0               ; 1 - get bit 0
        bld     r19, IEC_PIN_CLOCK ; 1 - send on the clock line
        bst     r0, 1               ; 1 - get bit 1
        bld     r19, IEC_PIN_DATA  ; 1 - send on the data line
        lsr     r0                  ; 1 - remove source bits
        lsr     r0                  ; 1
        out     IEC_OUT, r19 ; 1 - output the bit pair
        ret                         ; 4

        ;;
        ;; Sends a single byte using the Jiffy protocol
        ;; uint8_t value (r24), uint8_t eoi (r22), uint8_t loadflags (r20)
        ;; return uint8_t atnactive (r24)
        ;;
        ;; eoi should always be 0 for LOAD because the eoi marker is
        ;; transmitted in a different way there (see C code).
        ;;
        ;; Loadflags has a dual purpose:
        ;;  - if it's != 0, the start condition will be modified for Jiffy-LOAD
        ;;  - bit 7 will be cleared after that
        ;;  - if it's still != 0, the function will exit after sending
        ;;    the last bitpair - required for all but the final byte of a block
        ;;    during LOAD
        .global jiffy_send
jiffy_send:
        cli                     ; Disable interrupts

        ;; Calculate bus wait condition based on current state
        ;; The loop below will spin while ATN and Clock are high and
        ;; - Data is low  (single-byte transfer)
        ;; - Data is high (LOAD)
        ldi     r21, IEC_BIT_CLOCK | IEC_BIT_ATN
        tst     r20             ; loadflag set?
        breq    0f              ; Yes, skip
                ; No: Set data bit
        ori     r21, IEC_BIT_DATA

0:      andi    r20, 0x7f       ; Clear bit 7 of loadflags

        ;; Calculate bus state for EOI/not-EOI mark
        tst     r22             ; EOI flag set?
        breq    0f              ; No, branch

        in      r22, IEC_OUT
        andi    r22, ~(IEC_BIT_DATA|IEC_BIT_CLOCK|IEC_BIT_ATN)
        ori     r22, IEC_BIT_DATA ; Data low, Clock hi on EOI
        rjmp    1f

0:      in      r22, IEC_OUT
        andi    r22, ~(IEC_BIT_DATA|IEC_BIT_CLOCK|IEC_BIT_ATN)
        ori     r22, IEC_BIT_CLOCK ; Data hi, Clock low on non-EOI

1:
        ;; Set clock and data high/inactive - FFB5
        cbi     IEC_OUT, IEC_PIN_DATA
                ; this is the actual ready signal for the C64
        cbi     IEC_OUT, IEC_PIN_CLOCK

        ;; Wait 1us to allow the bus to settle (J1541 needs 4us here)
        rcall   delay1us

        ;; Set up a few registers
        mov     r0, r24         ; move data byte to r0
        com     r0              ; invert data byte

        ;; Wait for start condition (calculated above) from C64 - FFB8
0:      in      r24, IEC_IN
        andi    r24, IEC_BIT_CLOCK | IEC_BIT_DATA | IEC_BIT_ATN ; 1
        cp      r24, r21        ; 1
        breq    0b              ; 1

        ;; Check for ATN (J1541 doesn't do this, but I'm paranoid)
        ;; Doesn't work, further analysis required
        andi    r24, IEC_BIT_ATN ; 1
        breq    js_finish        ; 1 - jump if ATN is low

        ;; Output the first bitpair 6us (48 cycles) after the start
        ;; 6us is the best-case time from bus check to transmission in J1541
        delay_cycles 31 + JIFFY_OFFSET_WRITE
                                ; 31 = 48-17
        rcall   jiffy_sendbits  ; 12+4 - [FFBD]

        ;; Output the second bitpair 10us (80 cycles) after the first
        delay_cycles 64         ; 64 = 80-16

        rcall   jiffy_sendbits  ; 12+4 - [FFC4]

        ;; Output the third bitpair 11us (88 cycles) after the second
        delay_cycles 72         ; 72 = 88-16

        rcall   jiffy_sendbits  ; 12+4 - [FFCC]

        ;; Output the fourth bitpair 10us (80 cycles) after the third
        delay_cycles 64         ; 64 = 80-16

        rcall   jiffy_sendbits  ; 12+4 - [FFD3]

        ;; Skip sending EOI for LOAD code path
        tst     r20             ; 1
        brne    js_finish       ; 1

        ;; Output EOI marker 11us (88 cycles) after the last bitpair
        delay_cycles 81         ; 81 = 88-7

        out     IEC_OUT, r22 ; 1 - output EOI marker [FFDB]

        ;; Wait 1us to allow the bus to settle (J1541 needs 4us here)
        rcall   delay1us

        ;; Wait until data is low, check ATN [FFDE]
        ;; This loop won't wait when EOI is signalled.
0:      in      r24, IEC_IN ; Read bus
        sbrs    r24, IEC_PIN_ATN           ; Skip if ATN is high
        rjmp    js_finish                  ; ATN low, exit loop
        sbrc    r24, IEC_PIN_DATA          ; Skip if Data is low
        rjmp    0b                         ; No Data, no ATN -> loop

js_finish:
        com     r24             ; invert port state (ATN low returns true)
        andi    r24, IEC_BIT_ATN ; single out ATN bit

        sei                     ; re-enable interrupts
        ret


#if FL_TURBODISK
        ;; ====================================================================
        ;;  Turbodisk
        ;; ====================================================================

        ;;
        ;; Sends a single byte in r24 using the Turbodisk protocol
        ;;
        .global turbodisk_byte
turbodisk_byte:
        ;; Wait until data is low
        sbic    IEC_IN, IEC_PIN_DATA
        rjmp    turbodisk_byte

        ;; Byte preparation
        com     r24             ; Invert the data byte

        ;; All bus lines high
        in      r18, IEC_OUT
        andi    r18, ~(IEC_BIT_DATA|IEC_BIT_CLOCK|IEC_BIT_ATN)
        out     IEC_OUT, r18

        ;; Wait until data is high
0:      sbis    IEC_IN, IEC_PIN_DATA ; 2
        rjmp    0b

        rcall   delay1us        ; Move the timing window slightly

        ldi     r20, 4          ; 1 - Bitpair counter

tdbitloop:
        in      r19, IEC_OUT ; 1 - read & mask unused IEC port lines
        andi    r19, ~(IEC_BIT_DATA|IEC_BIT_CLOCK|IEC_BIT_ATN) ; 1
        bst     r24, 7              ; 1 - grab bit 7
        bld     r19, IEC_PIN_CLOCK ; 1 - store in clock bit
        bst     r24, 6              ; 1 - grab bit 6
        bld     r19, IEC_PIN_DATA  ; 1 - store in data bit
        lsl     r24                 ; 1 - remove source bits
        lsl     r24                 ; 1

        ;; Wait for 28us from data high -> 224-9=215 cycles
        delay_cycles 215        ; 215

        out     IEC_OUT, r19      ; 1 - output the bit pair

        rcall   delay1us        ; 8 - additional us for loops 2-4

        dec     r20             ; 1 - Decrement bitpair counter
        brne    tdbitloop       ; 2/1 - loop until done

        ;; Wait for 26us from last bitpair -> 208-12=196 cycles
        delay_cycles 196        ; 196

        ;; Data high, Clock low
        in      r18, IEC_OUT
        andi    r18, ~(IEC_BIT_DATA|IEC_BIT_CLOCK|IEC_BIT_ATN)
        ori     r18, IEC_BIT_CLOCK
        out     IEC_OUT, r18

        ret


        ;;
        ;; Sends a complete buffer of r22 bytes at r24/r25 using the Turbodisk protocol
        ;;
        .global turbodisk_buffer
turbodisk_buffer:
        ;; Wait until data is low
        sbic    IEC_IN, IEC_PIN_DATA
        rjmp    turbodisk_buffer

        ;; All bus lines high
        in      r18, IEC_OUT
        andi    r18, ~(IEC_BIT_DATA|IEC_BIT_CLOCK|IEC_BIT_ATN)
        out     IEC_OUT, r18

        movw    r30,r24         ; output pointer

        ;; Wait until data is high
0:      sbis    IEC_IN, IEC_PIN_DATA ; 2
        rjmp    0b

        ;; Initial delay of 4 us + move the timing window by 1 us for stability
        delay_cycles (4+1)*8    ; 40

        ;; 287 AVR cycles from here to first LDA $DD00 on C64 side ($F822)
tdbyteloop:
        ld      r0, Z+          ; 2 - load byte
        ldi     r20, 4          ; 1 - bitpair counter
        com     r0              ; 1 - invert data byte

        ;; Waste another 12 us -> 96-4=92 cycles
        delay_cycles 92         ; 92

tdbitloop2:
        in      r19, IEC_OUT ; 1 - read & mask unused IEC port lines
        andi    r19, ~(IEC_BIT_DATA|IEC_BIT_CLOCK|IEC_BIT_ATN) ; 1
        bst     r0, 7               ; 1 - grab bit 7
        bld     r19, IEC_PIN_CLOCK ; 1 - store in clock bit
        bst     r0, 6               ; 1 - grab bit 6
        bld     r19, IEC_PIN_DATA  ; 1 - store in data bit
        lsl     r0                  ; 1 - remove source bits
        lsl     r0                  ; 1

        ;; Wait another 24 us -> 192-9=183 cycles
        delay_cycles 183        ; 183

        out     IEC_OUT, r19      ; 1 - output the bit pair

        ;; 5 us -> 40-3=37 cycles
        delay_cycles 37         ; 37

        dec     r20             ; 1 - Decrement bitpair counter
        brne    tdbitloop2      ; 2/1 - loop until done

        ;; Wait 10 us -> 80-3+1=78 cycles
        delay_cycles 78         ; 78

        dec     r22             ; 1 - Decrement byte counter
        brne    tdbyteloop      ; 2/1 - loop until done

        ;; Final delay: 11 us -> 88 cycles
        delay_cycles 88         ; 88

        ;; Data high, Clock low
        in      r18, IEC_OUT
        andi    r18, ~(IEC_BIT_DATA|IEC_BIT_CLOCK|IEC_BIT_ATN)
        ori     r18, IEC_BIT_CLOCK
        out     IEC_OUT, r18

        ret
#endif

#if FL_DREAMLOAD || FL_FC3 || FL_ULOAD3 || FL_ELOAD1 || FL_GEOS
        ;;
        ;; Set bit 0 of r0 on CLOCK and bit 1 on DATA
        ;; A bit value of one means pull down
        ;; After sending these two bits r0 is shifted right two bits
        ;;
        ;; r19 is used as scratch reg here
        ;;
        ;; Takes 10 cycles before OUT (incl. rcall)
        ;; Takes 6 cycles to return after OUT
        ;;
send_bits_to_clk_data:
        ; rcall                  ; 3
        in   r19, IEC_OUT ; 1 - read & mask unused IEC port lines
        andi r19, ~(IEC_BIT_DATA|IEC_BIT_CLOCK|IEC_BIT_ATN) ; 1
        bst  r0, 0               ; 1 - take bit 0 (lsb)
        bld  r19, IEC_PIN_CLOCK ; 1 - for CLOCK line
        bst  r0, 1               ; 1 - take bit 1
        bld  r19, IEC_PIN_DATA  ; 1 - for DATA line
        out  IEC_OUT, r19 ; 1 - output the bit pair

        lsr  r0                  ; 1
        lsr  r0                  ; 1 - prepare next two bits

        ret                      ; 4

        ;;
        ;; Get DATA  => bit 0
        ;; and CLOCK => bit 2 of r24
        ;; Low level means value 0 here
        ;;
        ;; r19 is used as scratch reg here
        ;;
        ;; Takes 4 cycles before IN (incl. rcall)
        ;; Takes 8 cycles to return after IN
        ;;
get_bits_from_clk_data:
        ; rcall                         ; 3
        in      r19, IEC_IN  ; 1
        bst     r19, IEC_PIN_DATA       ; 1
        bld     r24, 0                  ; 1
        bst     r19, IEC_PIN_CLOCK      ; 1
        bld     r24, 2                  ; 1
        ret                             ; 4

#endif

#if FL_FC3 || FL_ULOAD3 || FL_ELOAD1
        ;;
        ;; Do this kind of handshake:
        ;; set CLOCK low, wait for DATA going low,
        ;; release CLOCK, wait for DATA going high
        ;; returns without waiting if ATN is low
        ;;
        ;; cycles from DATA high to return to caller: 5-10
        ;;
        .global clk_data_handshake
clk_data_handshake:
        sbi     IEC_OUT, IEC_PIN_CLOCK
1:
        sbis    IEC_IN, IEC_PIN_ATN
        rjmp    2f
        sbic    IEC_IN, IEC_PIN_DATA
        rjmp    1b
        cbi     IEC_OUT, IEC_PIN_CLOCK
1:
        sbis    IEC_IN, IEC_PIN_ATN
        rjmp    2f
        sbis    IEC_IN, IEC_PIN_DATA
        rjmp    1b
2:
        ret
#endif

#if FL_FC3
        ;;
        ;; Sends four bytes at r24/r25 using the FC3 protocol
        ;;
        .global fastloader_fc3_send_block
fastloader_fc3_send_block:
        cli
        movw    r30,r24         ; Z = output pointer

        ; pull CLOCK low for sync
        sbi     IEC_OUT, IEC_PIN_CLOCK ; 1

        ; 12 us between sync and bitpair
        delay_cycles 12 * 8 - 14 ; 83

        ldi     r21, 4          ; 1 - byte counter
fc3_byteloop:
        ld      r0, Z+          ; 2 - load byte

        com     r0              ; 1 - negate all bits

        ldi     r20, 4          ; 1 - bitpair counter
fc3_bitloop:
        ; send r0[0,1] to CLOCK and DATA, shift r0 right two times
        ; changes r19
        rcall   send_bits_to_clk_data ; 10 + 6

        ; wait 12 us between bitpair
        delay_cycles 12*8-19

        dec     r20             ; 1 - decrement bitpair counter
        brne    fc3_bitloop     ; 2/1 - loop until done
fc3_next_byte:
        ; wait 2 us more between two bytes (16 cycles = -1 + 17)
        rcall   delay1us        ; 8
        rjmp    0f              ; 2
0:
        dec     r21             ; 1 - decrement byte counter
        brne    fc3_byteloop    ; 2/1 - loop until done

        ; release CLOCK and DATA
        in      r19, IEC_OUT ; read & mask unused IEC port lines
        andi    r19, ~(IEC_BIT_DATA|IEC_BIT_CLOCK|IEC_BIT_ATN)
        out     IEC_OUT, r19 ; release CLOCK and DATA

        sei
        ret

        ;;
        ;; Receive one byte using the FC3 save protocol
        ;;
        .global fc3_get_byte
fc3_get_byte:
        cli

        clr     r24     ; clear data byte

        ; wait a moment before releasing data, otherwise the loop would run
        ; too fast. I confess that this value is guessed.
        delay_cycles 10 * 8

        ; release DATA
        cbi     IEC_OUT, IEC_PIN_DATA

        ; wait until CLOCK goes high
1:
        sbis    IEC_IN, IEC_PIN_CLOCK ; 1/2?
        rjmp    1b

        ; 17 us later...
        delay_cycles 17 * 8 - 6

        ; get bits 5,7 @ 0,2 - changes r19
        rcall   get_bits_from_clk_data  ; 4 + 8 r24 = .....7.5
        lsl     r24                     ; 1     r24 = ....7.5.

        ; 13 us later...
        delay_cycles 13 * 8 - 13

        ; get bits 4,6 @ 0,2 - changes r19
        rcall   get_bits_from_clk_data  ; 4 + 8 r24 = ....7654
        swap    r24                     ; 1     r24 = 7654....
        lsr     r24                     ; 1     r24 = .7654...

        ; 12 us later...
        delay_cycles 12 * 8 - 14

        ; get bits 1,3 @ 0,2 - changes r19
        rcall   get_bits_from_clk_data  ; 4 + 8 r24 = .76543.1
        lsl     r24                     ; 1     r24 = 76543.1.

        ; 10 us later...
        delay_cycles 10 * 8 - 13

        ; get bits 0,2 @ 0,2 - changes r19
        rcall   get_bits_from_clk_data  ; 4 + 8 r24 = 76543210

        ; Pull DATA down
        sbi     IEC_OUT, IEC_PIN_DATA

        sei
        com     r24     ; negate all bits
        clr     r25     ; clear hi-byte of return val
        ret


        ;;
        ;; Send one byte using the old FC3 freezer protocol, PAL version
        ;;
        .global fc3_oldfreeze_pal_send
fc3_oldfreeze_pal_send:
        cli
        mov     r0, r24         ; bitpair subroutine expects data in r0

        ;; clear busy signal
        cbi     IEC_OUT, IEC_PIN_DATA

        ;; wait for start signal (clock low->high)
1:      sbic    IEC_IN, IEC_PIN_CLOCK
        rjmp    1b

2:      sbis    IEC_IN, IEC_PIN_CLOCK
        rjmp    2b

        ;; send bits 0+1
        delay_cycles 14 * 8 - 10
        rcall   send_bits_to_clk_data ; 10+6

        ;; send bits 2+3
        delay_cycles (22 - 14) * 8 - 16
        rcall   send_bits_to_clk_data ; 10+6

        ;; send bits 4+5
        delay_cycles (30 - 22) * 8 - 16
        rcall   send_bits_to_clk_data ; 10+6

        ;; send bits 6+7
        delay_cycles (38 - 30) * 8 - 16
        rcall   send_bits_to_clk_data ; 10+6

        ;; hold time before re-enabling busy
        delay_cycles (46 - 38) * 8 - 6 - 1
        cbi     IEC_OUT, IEC_PIN_CLOCK ; 1
        sbi     IEC_OUT, IEC_PIN_DATA  ; 1
        rcall   delay1us

        ;; return inverted ATN state in r24
        in      r24, IEC_IN
        com     r24
        andi    r24, IEC_BIT_ATN
        sei
        ret


        ;;
        ;; Send one byte using the old FC3 freezer protocol, NTSC version
        ;;
        .global fc3_oldfreeze_ntsc_send
fc3_oldfreeze_ntsc_send:
        cli
        mov     r0, r24         ; bitpair subroutine expects data in r0

        ;; clear busy signal
        cbi     IEC_OUT, IEC_PIN_DATA

        ;; wait for start signal (clock low->high)
1:      sbic    IEC_IN, IEC_PIN_CLOCK
        rjmp    1b

2:      sbis    IEC_IN, IEC_PIN_CLOCK
        rjmp    2b

        ;; send bits 0+1
        delay_cycles 14 * 8 - 10
        rcall   send_bits_to_clk_data ; 10+6

        ;; send bits 2+3
        delay_cycles (24 - 14) * 8 - 16
        rcall   send_bits_to_clk_data ; 10+6

        ;; send bits 4+5
        delay_cycles (34 - 24) * 8 - 16
        rcall   send_bits_to_clk_data ; 10+6

        ;; send bits 6+7
        delay_cycles (44 - 34) * 8 - 16
        rcall   send_bits_to_clk_data ; 10+6

        ;; hold time before re-enabling busy
        delay_cycles (52 - 44) * 8 - 6 - 1
        cbi     IEC_OUT, IEC_PIN_CLOCK ; 1
        sbi     IEC_OUT, IEC_PIN_DATA  ; 1
        rcall   delay1us

        ;; return inverted ATN state in r24
        in      r24, IEC_IN
        com     r24
        andi    r24, IEC_BIT_ATN
        sei
        ret
#endif


#if FL_DREAMLOAD
        ;;
        ;; Receive a command (track/sector) using the DreamLoad protocol
        ;;
dreamload_get_command:
        rcall   dreamload_get_byte
        sts     fl_track, r24
        rcall   dreamload_get_byte
        sts     fl_sector, r24
        ret

        ;;
        ;; Receive a byte using the DreamLoad protocol
        ;;
        .global dreamload_get_byte
dreamload_get_byte:

        ldi     r18, 4  ; 4 * 2 bits to load
dl_bitloop:
        ; wait until clock goes low
1:
        sbic    IEC_IN, IEC_PIN_CLOCK
        rjmp    1b

        lsl     r24

        ; read data a short time later
        in      r19, IEC_IN
        bst     r19, IEC_PIN_DATA
        bld     r24, 0

        ; wait until clock goes high
2:
        sbis    IEC_IN, IEC_PIN_CLOCK
        rjmp    2b

        lsl     r24

        ; read data a short time later
        in      r19, IEC_IN
        bst     r19, IEC_PIN_DATA
        bld     r24, 0

        dec     r18
        brne     dl_bitloop

        com     r24     ; negate all bits
        ret

        ;;
        ;; Receive a command (track/sector) using the old DreamLoad protocol
        ;;
dreamload_get_command_old:
        rcall   dreamload_get_byte_old
        sts     fl_track, r24
        rcall   dreamload_get_byte_old
        sts     fl_sector, r24
        ret

        ;;
        ;; Receive a byte using the old DreamLoad protocol
        ;;
dreamload_get_byte_old:
        ; 2 nibbles
        ldi     r18, 2
dlgb_nibble:
        swap    r24                     ; 2nd loop: r24 = 7654....
        lsr     r24                     ; 2nd loop: r24 = .7654...

        rcall   wait_atn_low
        ; 1st: get bits 5,7 @ 0,2 / 2nd: get bits 1,3 @ 0,2 - changes r19
        rcall   get_bits_from_clk_data  ; 1st: r24 = .....7.5 / 2nd: .76543.1
        lsl     r24                     ; 1st: r24 = ....7.5. / 2nd: 76543.1.

        rcall   wait_atn_high
        ; 1st: get bits 4,6 @ 0,2 / 2nd: get bits 0,2 @ 0,2 - changes r19
        rcall   get_bits_from_clk_data  ; 1st: r24 = ....7654 / 2nd: 76543210

        dec     r18
        brne    dlgb_nibble

        com     r24     ; negate all bits
        ret

        ;;
        ;; Send the byte in r24 using the DreamLoad protocol
        ;;
        .global dreamload_send_byte
dreamload_send_byte:

        ; inverse all bits
        mov     r0, r24
        com     r0

        ldi     r18, 2          ; loop counter

dsb_bitloop:
        ; send r0[0,1] to CLOCK and DATA, shift r0 right two times
        ; changes r19
        rcall   send_bits_to_clk_data
        rcall   wait_atn_low

        ; send r0[0,1] to CLOCK and DATA, shift r0 right two times
        ; changes r19
        rcall   send_bits_to_clk_data
        rcall   wait_atn_high

        dec     r18             ; decrement bitpair counter
        brne    dsb_bitloop     ; loop until done
        ret
#endif

#if FL_ULOAD3 || FL_ELOAD1
        ;;
        ;; Receive a byte using the ULoad Model 3 protocol
        ;;
        .global uload3_get_byte
uload3_get_byte:
        cli
        ldi     r25, 0xff       ; prepare a negative return value
        clr     r24
        rcall   clk_data_handshake
        sbis    IEC_IN, IEC_PIN_ATN ;skip ret if ATN is high
        rjmp    uload3_abort                       ;return if ATN is low

        ;; assuming an average of 10 cycles between DATA high and the following instruction
        delay_cycles 14 * 8 - 10 - 4

        ;; read bits 7/5
        rcall   get_bits_from_clk_data ;4+8 r24 = _____7_5
        lsl     r24                    ;1   r24 = ____7_5_

        delay_cycles 10 * 8 - 13

        ;; read bits 6/4
        rcall   get_bits_from_clk_data ;4+8 r24 = ____7654
        swap    r24                    ;1   r24 = 7654____
        lsr     r24                    ;1   r24 = _7654___

        delay_cycles 14 * 8 - 14

        ;; read bits 3/1
        rcall   get_bits_from_clk_data ;4+8 r24 = _76543_1
        lsl     r24                    ;1   r24 = 76543_1_

        delay_cycles 10 * 8 - 13

        ;; read bits 2/0
        rcall   get_bits_from_clk_data ;4+8 r24 = 76543210

        delay_cycles 20*8       ; wait a bit so the C64 can return the bus to idle state

        clr     r25             ; clear upper byte of return value
        com     r24             ; invert data byte

uload3_abort:
        sei
        ret


        ;;
        ;; Send a byte using the ULoad Model 3 protocol
        ;;
        .global uload3_send_byte
uload3_send_byte:
        cli

        ;; DATA low
        sbi     IEC_OUT, IEC_PIN_DATA

        ;; wait for CLOCK low
1:      sbis    IEC_IN, IEC_PIN_ATN
        rjmp    2f
        sbic    IEC_IN, IEC_PIN_CLOCK
        rjmp    1b

        ;; DATA high
        cbi     IEC_OUT, IEC_PIN_DATA

        ;; wait for CLOCK high - 2 to 7 cycles, assuming 5
1:
        sbis    IEC_IN, IEC_PIN_ATN
        rjmp    2f
        sbis    IEC_IN, IEC_PIN_CLOCK
        rjmp    1b

        ;; prepare transmission
        mov     r0, r24         ;1 - move data to output register
        com     r0              ;1 - invert byte
        ldi     r24, 4          ;1 - number of loops
        delay_cycles (14-8) * 8 - 8 + 6 + 3 ; initial delay before first bit pair

uload3_bitloop:
        delay_cycles 8 * 8 - 16 - 3   ; delay between bit pairs
        rcall   send_bits_to_clk_data ;10+6
        dec     r24                   ;1 - decrement pair counter
        brne    uload3_bitloop        ;2 - continue loop

        delay_cycles 10*8 - 9   ; delay before releasing clock/data

        ;; release CLOCK+DATA
        cbi     IEC_OUT, IEC_PIN_CLOCK
        cbi     IEC_OUT, IEC_PIN_DATA
        rcall   delay1us        ; safety
2:
        sei
        ret
#endif

#if FL_EPYXCART
        ;; send bits 7 and 5 of r0 to clock/data
        ;; masked contents of IEC_OUTPUT expected in r19
        ;; 8 cycles from rcall to out, 4 to return
epyx_bitpair:
        ;; rcall - 3
        bst     r0, 7                      ; 1
        bld     r19, IEC_PIN_CLOCK        ; 1
        bst     r0, 5                      ; 1
        bld     r19, IEC_PIN_DATA         ; 1
        out     IEC_OUT, r19 ; 1
        ret                                ; 4

        ;;
        ;; Send a byte using the Epyx Fastload cartridge protocol
        ;;
        .global epyxcart_send_byte
epyxcart_send_byte:
        ;; DATA and CLOCK high
        cbi     IEC_OUT, IEC_PIN_DATA
        cbi     IEC_OUT, IEC_PIN_CLOCK
        rcall   delay1us

        ;; prepare data
        in      r19, IEC_OUT
        andi    r19, ~(IEC_BIT_DATA|IEC_BIT_CLOCK|IEC_BIT_ATN)

        ;; wait for DATA high or ATN low
1:      sbis    IEC_IN, IEC_PIN_ATN
        rjmp    epyxcart_atnabort
        sbis    IEC_IN, IEC_PIN_DATA
        rjmp    1b

        mov     r0, r24             ; 1
        delay_cycles (10*8 - 9)
        rcall   epyx_bitpair        ; 8+4 - bits 7 and 5

        lsl     r0                  ; 1
        delay_cycles (10*8 - 13)
        rcall   epyx_bitpair        ; 8+4 - bits 6 and 4

        swap    r24                 ; 1
        mov     r0, r24             ; 1
        delay_cycles (10*8 - 14)
        rcall   epyx_bitpair        ; 8+4 - bits 3 and 1

        lsl     r0                  ; 1
        delay_cycles (10*8 - 13)
        rcall   epyx_bitpair        ; 8+4 - bits 2 and 0

        delay_cycles 20*8  ; final delay so the data stays valid long enough

        clr     r24
        ret

epyxcart_atnabort:
        ldi     r24, 1
        ret

#endif

#if FL_GEOS
        ;;
        ;; read a byte using the GEOS protocol (common part)
        ;;
geos_get_byte_common:
        clr     r24

        ;; wait until clock is high
1:      sbis    IEC_IN, IEC_PIN_CLOCK
        rjmp    1b

        ;; wait until clock is low
1:      sbic    IEC_IN, IEC_PIN_CLOCK ; 1/2
        rjmp    1b

        ;; delay
        delay_cycles 15*8 - 2

        ;; get bits 4 and 5
        in      r19, IEC_IN ; 1
        bst     r19, IEC_PIN_CLOCK         ; 1
        bld     r24, 4-1                   ; 1 - r24 = ....4...
        bst     r19, IEC_PIN_DATA          ; 1
        bld     r24, 5-1                   ; 1 - r24 = ...54...

        ;; delay
        delay_cycles 14*8 - 5

        ;; get bits 6 and 7
        in      r19, IEC_IN ; 1
        bst     r19, IEC_PIN_CLOCK         ; 1
        bld     r24, 6-1                   ; 1 - r24 = ..654...
        bst     r19, IEC_PIN_DATA          ; 1
        bld     r24, 7-1                   ; 1 - r24 = .7654...

        ret                                ; 4

        ;;
        ;; read a byte using the GEOS 1MHz (1541) protocol
        ;;
        .global geos_get_byte_1mhz
geos_get_byte_1mhz:
        rcall   geos_get_byte_common   ; 3

        ;; delay
        delay_cycles 14*8 - 9 - 4

        ;; get bits 3 and 1
        rcall   get_bits_from_clk_data ; 4+8 - r24 = .76543.1
        lsl     r24                    ; 1     r24 = 76543.1.

        ;; delay
        delay_cycles 16*8 - 9 - 4

        ;; get bits 2 and 0
        rcall   get_bits_from_clk_data ; 4+8 - r24 = 76543210

        ;; post-process received value and return
        delay_cycles 11*8
        com     r24             ; negate all bits
        clr     r25             ; clear high byte of return value
        ret


        ;;
        ;; read a byte using the GEOS 2MHz (1571/81) protocol
        ;;
        .global geos_get_byte_2mhz
geos_get_byte_2mhz:
        rcall   geos_get_byte_common   ; 3

        ;; delay
        delay_cycles 19*4 - 9 - 4

        ;; get bits 3 and 1
        rcall   get_bits_from_clk_data ; 4+8 - r24 = .76543.1
        lsl     r24                    ; 1     r24 = 76543.1.

        ;; delay
        delay_cycles 22*4 - 9 - 4

        ;; get bits 2 and 0
        rcall   get_bits_from_clk_data ; 4+8 - r24 = 76543210

        ;;  post process received value and return
        delay_cycles 11*8
        com     r24             ; negate all bits
        clr     r25             ; clear high byte of return value
        ret


        ;;
        ;; common part of all geos_send_byte implementations
        ;;  returns 7 cycles after clock low
        ;;
geos_send_byte_common:
        ;; CLOCK/DATA high
        cbi     IEC_OUT, IEC_PIN_CLOCK
        cbi     IEC_OUT, IEC_PIN_DATA
        rcall   delay1us

        ;; wait until CLOCK is low
1:      sbic    IEC_IN, IEC_PIN_CLOCK
        rjmp    1b

        ;; preserve non-IEC lines on IEC_OUTPUT port
        in      r19, IEC_OUT ; 1
        andi    r19, ~(IEC_BIT_DATA|IEC_BIT_CLOCK|IEC_BIT_ATN) ; 1
        mov     r0, r19             ; 1
        ret                         ; 4

        ;;
        ;; send a byte using the GEOS 1MHz protocol
        ;; called with byte in r24, return value undefined
        ;;
        .global geos_send_byte_1mhz
geos_send_byte_1mhz:
        rcall   geos_send_byte_common ; 7

        ;; prepare bits 3+1
        bst     r24, 3              ; 1 - read bit 3
        bld     r19, IEC_PIN_CLOCK ; 1 - store in clock bit
        bst     r24, 1              ; 1 - read bit 1
        bld     r19, IEC_PIN_DATA  ; 1 - store in data bit

        ;; delay (1MHz speeder)
        delay_cycles 18*8 - 14
        rjmp    geos_send_byte_tail   ; 2


        ;;
        ;; send a byte using the GEOS 2MHz protocol
        ;; called with byte in r24, return value undefined
        ;;
        .global geos_send_byte_2mhz
geos_send_byte_2mhz:
        rcall   geos_send_byte_common ; 7

        ;; prepare bits 3+1
        bst     r24, 3              ; 1 - read bit 3
        bld     r19, IEC_PIN_CLOCK ; 1 - store in clock bit
        bst     r24, 1              ; 1 - read bit 1
        bld     r19, IEC_PIN_DATA  ; 1 - store in data bit

        ;; delay (2MHz speeder)
1:      delay_cycles 9*8 - 12

geos_send_byte_tail:
        ;; send to bus
        out     IEC_OUT, r19 ; 1 - send to bus

        ;; prepare bits 2+0
        mov     r19, r0             ; 1
        bst     r24, 2              ; 1 - read bit 2
        bld     r19, IEC_PIN_CLOCK ; 1 - store in clock bit
        bst     r24, 0              ; 1 - read bit 0
        bld     r19, IEC_PIN_DATA  ; 1 - store in data bit

        ;; delay
        delay_cycles 10*8 - 6

        ;; send to bus
        out     IEC_OUT, r19 ; 1

        ;; prepare bits 4+5
        com     r24                 ; 1 - remaining bits are sent with high=1
        mov     r19, r0             ; 1
        bst     r24, 4              ; 1 - read bit 4
        bld     r19, IEC_PIN_CLOCK ; 1 - store in clock bit
        bst     r24, 5              ; 1 - read bit 5
        bld     r19, IEC_PIN_DATA  ; 1 - store in data bit

        ;; delay
        delay_cycles 11*8 - 7

        ;; send to bus
        out     IEC_OUT, r19 ; 1

        ;; prepare bits 6+7
        mov     r19, r0             ; 1
        bst     r24, 6              ; 1 - read bit 6
        bld     r19, IEC_PIN_CLOCK ; 1 - store in clock bit
        bst     r24, 7              ; 1 - read bit 7
        bld     r19, IEC_PIN_DATA  ; 1 - store in data bit

        ;; delay
        delay_cycles 12*8 - 6

        ;; send to bus
        out     IEC_OUT, r19 ; 1

        ;; final delay
        delay_cycles 22*8

        ret


        ;;
        ;; send a byte using the 2MHz 1581 Configure 2.1 GEOS protocol
        ;; called with byte in r24, return value undefined
        ;;
        .global geos_send_byte_1581_21
geos_send_byte_1581_21:
        com     r24                   ; invert so the bits are sent with high=1
        rcall   geos_send_byte_common ; 7
        mov     r0, r24               ; 1 - move data to target register

        ;; delay
        delay_cycles 7*8 - 18

        ;; send bits 0+1
        rcall   send_bits_to_clk_data ; 10+6

        ;; delay
        delay_cycles 7*8 - 16

        ;; send bits 2+3
        rcall   send_bits_to_clk_data ; 10+6

        ;; delay
        delay_cycles 10*8 - 16

        ;; send bits 4+5
        rcall   send_bits_to_clk_data ; 10+6

        ;; delay
        delay_cycles 9*8 - 16

        ;; send bits 6+7
        rcall   send_bits_to_clk_data ; 10+6

        ;; final delay
        delay_cycles 12*8

        ret

#endif

#if FL_WHEELS
        ;;
        ;; send a byte using the Wheels protocol
        ;; called with byte in r24, return value undefined
        ;;
        .global wheels_send_byte_1mhz
wheels_send_byte_1mhz:
        rcall   geos_send_byte_common ; 7

        rcall   wheels_nibble       ; 3 - send lower nibble
        swap    r24                 ; 1 - swap nibbles

        delay_cycles (14-9)*8 + 15 - 14
        rcall   wheels_nibble       ; 3 - send upper nibble

        ;; final delay
        delay_cycles 22*8
        ret

wheels_nibble:
        ;; prepare bits 3+1 (7+5)
        mov     r19, r0             ; 1
        bst     r24, 3              ; 1 - read bit 3
        bld     r19, IEC_PIN_CLOCK ; 1 - store in clock bit
        bst     r24, 1              ; 1 - read bit 1
        bld     r19, IEC_PIN_DATA  ; 1 - store in data bit

        ;; delay
        delay_cycles 9*8 - 15

        ;; send to bus
        out     IEC_OUT, r19 ; 1 - send to bus

        ;; prepare bits 2+0 (6+4)
        mov     r19, r0             ; 1
        bst     r24, 2              ; 1 - read bit 2
        bld     r19, IEC_PIN_CLOCK ; 1 - store in clock bit
        bst     r24, 0              ; 1 - read bit 0
        bld     r19, IEC_PIN_DATA  ; 1 - store in data bit

        ;; delay
        delay_cycles 14*8 - 6

        ;; send to bus
        out     IEC_OUT, r19 ; 1 - send to bus
        ret                                ; 4


        ;;
        ;; read a byte using the Wheels protocol
        ;;
        .global wheels_get_byte_1mhz
wheels_get_byte_1mhz:
        clr     r24

        ;; wait until clock is high
1:      sbis    IEC_IN, IEC_PIN_CLOCK
        rjmp    1b

        ;; wait until clock is low
1:      sbic    IEC_IN, IEC_PIN_CLOCK ; 1/2
        rjmp    1b

        ;; delay
        delay_cycles 16 * 8 - 5

        ;; read bits 7/5
        rcall   get_bits_from_clk_data ; 4+8 r24 = _____7_5
        lsl     r24                    ; 1   r24 = ____7_5_

        delay_cycles 10 * 8 - 13

        ;; read bits 6/4
        rcall   get_bits_from_clk_data ; 4+8 r24 = ____7654
        swap    r24                    ; 1   r24 = 7654____
        lsr     r24                    ; 1   r24 = _7654___

        delay_cycles 15 * 8 - 14

        ;; read bits 3/1
        rcall   get_bits_from_clk_data ; 4+8 r24 = _76543_1
        lsl     r24                    ; 1   r24 = 76543_1_

        delay_cycles 13 * 8 - 13

        ;; read bits 2/0
        rcall   get_bits_from_clk_data ; 4+8 r24 = 76543210

        delay_cycles 20*8

        clr     r25             ; clear upper byte of return value
        com     r24             ; invert data byte

        ret

        ;;
        ;; read a byte using the Wheels 4.4 1MHz protocol
        ;;
        .global wheels44_get_byte_1mhz
wheels44_get_byte_1mhz:
        clr     r24

        ;; wait until clock is high
1:      sbis    IEC_IN, IEC_PIN_CLOCK
        rjmp    1b

        ;; wait until clock is low
1:      sbic    IEC_IN, IEC_PIN_CLOCK ; 1/2
        rjmp    1b

        ;; delay
        delay_cycles 17 * 8 - 5

        ;; read bits 7/5
        rcall   get_bits_from_clk_data ; 4+8 r24 = _____7_5
        lsl     r24                    ; 1   r24 = ____7_5_

        delay_cycles 11 * 8 - 13

        ;; read bits 6/4
        rcall   get_bits_from_clk_data ; 4+8 r24 = ____7654
        swap    r24                    ; 1   r24 = 7654____
        lsr     r24                    ; 1   r24 = _7654___

        delay_cycles 17 * 8 - 14

        ;; read bits 3/1
        rcall   get_bits_from_clk_data ; 4+8 r24 = _76543_1
        lsl     r24                    ; 1   r24 = 76543_1_

        delay_cycles 16 * 8 - 13

        ;; read bits 2/0
        rcall   get_bits_from_clk_data ; 4+8 r24 = 76543210

        delay_cycles 20*8

        clr     r25             ; clear upper byte of return value
        com     r24             ; invert data byte

        ret

        ;;
        ;; read a byte using the Wheels 4.4 2MHz protocol
        ;;
        .global wheels44_get_byte_2mhz
wheels44_get_byte_2mhz:
        clr     r24

        ;; wait until clock is high
1:      sbis    IEC_IN, IEC_PIN_CLOCK
        rjmp    1b

        ;; wait until clock is low
1:      sbic    IEC_IN, IEC_PIN_CLOCK ; 1/2
        rjmp    1b

        ;; delay
        delay_cycles 15*8 - 2

        ;; get bits 0 and 1
        in      r19, IEC_IN ; 1
        bst     r19, IEC_PIN_CLOCK         ; 1
        bld     r24, 0                     ; 1 - r24 = .......0
        bst     r19, IEC_PIN_DATA          ; 1
        bld     r24, 1                     ; 1 - r24 = ......10

        ;; delay
        delay_cycles 11*8 - 5

        ;; get bits 2 and 3
        in      r19, IEC_IN ; 1
        bst     r19, IEC_PIN_CLOCK         ; 1
        bld     r24, 2                     ; 1 - r24 = .....210
        bst     r19, IEC_PIN_DATA          ; 1
        bld     r24, 3                     ; 1 - r24 = ....3210

        ;; delay
        delay_cycles 11*8 - 5

        ;; get bits 4 and 5
        in      r19, IEC_IN ; 1
        bst     r19, IEC_PIN_CLOCK         ; 1
        bld     r24, 4                     ; 1 - r24 = ...43210
        bst     r19, IEC_PIN_DATA          ; 1
        bld     r24, 5                     ; 1 - r24 = ..543210

        ;; delay
        delay_cycles 11*8 - 5

        ;; get bits 6 and 7
        in      r19, IEC_IN ; 1
        bst     r19, IEC_PIN_CLOCK         ; 1
        bld     r24, 6                     ; 1 - r24 = .6543210
        bst     r19, IEC_PIN_DATA          ; 1
        bld     r24, 7                     ; 1 - r24 = 76543210

        ;; safety delay
        delay_cycles 12*8

        ;; post process received value and return
        com     r24                  ; negate all bits
        clr     r25                  ; clear high byte of return value
        ret


        ;;
        ;; send a byte using the Wheels 4.4 2MHz protocol
        ;; called with byte in r24, return value undefined
        ;;
        .global wheels44_send_byte_2mhz
wheels44_send_byte_2mhz:
        com     r24                   ; invert so the bits are sent with high=1
        rcall   geos_send_byte_common ; 7
        mov     r0, r24               ; 1 - move data to target register

        ;; delay
        delay_cycles 7*8 - 18

        ;; send bits 0+1
        rcall   send_bits_to_clk_data ; 10+6

        ;; delay
        delay_cycles 8*8 - 16

        ;; send bits 2+3
        rcall   send_bits_to_clk_data ; 10+6

        ;; delay
        delay_cycles 11*8 - 16

        ;; send bits 4+5
        rcall   send_bits_to_clk_data ; 10+6

        ;; delay
        delay_cycles 11*8 - 16

        ;; send bits 6+7
        rcall   send_bits_to_clk_data ; 10+6

        ;; final delay
        delay_cycles 14*8

        ret

#endif

#if FL_AR6
        ;;
        ;; send a byte using the AR6-1581 protocol
        ;; called with byte in r24, no return value
        ;;
        .global ar6_1581_send_byte
ar6_1581_send_byte:
        cli

        ;; set up data byte for transmission
        com     r24
        mov     r0, r24

        ;; clock high
        cbi     IEC_OUT, IEC_PIN_CLOCK

        ;; wait until data is high
1:      sbis    IEC_IN, IEC_PIN_DATA
        rjmp    1b

        ;; delay
        delay_cycles 5*8 - 10

        ;; send bits 0+1
        rcall   send_bits_to_clk_data ; 10+6

        ;; delay
        delay_cycles 8*8 - 16

        ;; send bits 2+3
        rcall   send_bits_to_clk_data ; 10+6

        ;; delay
        delay_cycles 8*8 - 16

        ;; send bits 4+5
        rcall   send_bits_to_clk_data ; 10+6

        ;; delay
        delay_cycles 8*8 - 16

        ;; send bits 6+7
        rcall   send_bits_to_clk_data ; 10+6

        ;; delay
        delay_cycles 8*8+4 - 6 - 2

        ;; set clock low, data high
        cbi     IEC_OUT, IEC_PIN_DATA  ; 1
        sbi     IEC_OUT, IEC_PIN_CLOCK ; 1

        sei
        ret

        ;;
        ;; receive a byte using the AR6-1581-PAL protocol
        ;; returns byte in r24
        ;;
        .global ar6_1581p_get_byte
ar6_1581p_get_byte:
        cli

        clr     r24

        ;; clock high
        cbi     IEC_OUT, IEC_PIN_CLOCK

        ;; wait until data is low
1:      sbic    IEC_IN, IEC_PIN_DATA
        rjmp    1b

        ;; wait until data is high
1:      sbis    IEC_IN, IEC_PIN_DATA
        rjmp    1b

        ;; delay
        delay_cycles 12*8 - 4

        ;; receive bits 7+5
        rcall   get_bits_from_clk_data ; 4+8 - r24 = .....7.5
        lsl     r24                    ; 1   - r24 = ....7.5.

        ;; delay
        delay_cycles 10*8 - 9 - 4

        ;; receive bits 6+4
        rcall   get_bits_from_clk_data ; 4+8 - r24 = ....7654
        swap    r24                    ; 1   - r24 = 7654....
        lsr     r24                    ; 1   - r24 = .7654...

        ;; delay
        delay_cycles 16*8 - 10 - 4

        ;; receive bits 3+1
        rcall   get_bits_from_clk_data ; 4+8 - r24 = .76543.1
        lsl     r24                    ; 1   - r24 = 76543.1.

        ;; delay
        delay_cycles 10*8 - 9 - 4

        ;; receive bits 2+0
        rcall   get_bits_from_clk_data ; 4+8 - r24 = 76543210

        ;; delay
        delay_cycles 5*8 - 8 - 1

        ;; set clock low
        sbi     IEC_OUT, IEC_PIN_CLOCK

        ;; post-processing
        sei
        com     r24
        clr     r25
        ret
#endif

#if FL_N0SDOS
        ;; ====================================================================
        ;;  N0stalgia loaders
        ;; ====================================================================

        ;; void n0sdos_send_byte(uint8_t byte)
        .global n0sdos_send_byte
n0sdos_send_byte:
        cli

        ;; set up data byte for transmission
        mov     r0, r24

        ;; clock and data high
        cbi     IEC_OUT, IEC_PIN_CLOCK
        cbi     IEC_OUT, IEC_PIN_DATA

        ;; wait until clock is high
1:      sbis    IEC_IN, IEC_PIN_CLOCK
        rjmp    1b

        ;; delay
        delay_cycles 9*8 - 10

        ;; send bits 0+1
        rcall   send_bits_to_clk_data ; 10+6

        ;; delay
        delay_cycles 8*8 - 16

        ;; send bits 2+3
        rcall   send_bits_to_clk_data ; 10+6

        ;; delay
        delay_cycles 8*8 - 16

        ;; send bits 4+5
        rcall   send_bits_to_clk_data ; 10+6

        ;; delay
        delay_cycles 8*8 - 16

        ;; send bits 6+7
        rcall   send_bits_to_clk_data ; 10+6

        ;; delay
        delay_cycles 5*8 - 6 - 2

        ;; set clock high, data low
        cbi     IEC_OUT, IEC_PIN_CLOCK ; 1
        sbi     IEC_OUT, IEC_PIN_DATA  ; 1

        delay_cycles 6*8

        sei
        ret

#endif

        .end
