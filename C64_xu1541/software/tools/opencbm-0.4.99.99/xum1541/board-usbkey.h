/*
 * Board interface for the USBKEY development kit
 * Copyright (c) 2009-2010 Nate Lawson <nate@root.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#ifndef _BOARD_USBKEY_H
#define _BOARD_USBKEY_H

// Initialize the board (timer, indicators, UART)
void board_init(void);
// Initialize the IO ports for IEC mode
void board_init_iec(void);

/*
 * Mapping of iec lines to IO port signals.
 *
 * NOTE: the XAP1541 adapter Nate is using has separate I/O pins
 * for inputs and outputs, so we depend on the IN signal bits being
 * the OUT signal << 1.
 *
 * The below defined pins are the OUT signals only but we derive the
 * input pin numbers from them.
 */
#define IO_ATN          _BV(0)
#define IO_CLK          _BV(2)
#define IO_DATA         _BV(4)
#define IO_RESET        _BV(6)
#define IO_OUTPUT_MASK  (IO_ATN | IO_CLK | IO_DATA | IO_RESET)

// IEC and parallel port accessors
#define CBM_PORT        PORTA
#define CBM_DDR         DDRA
#define CBM_PIN         PINA

#define PAR_PORT_PORT   PORTC
#define PAR_PORT_DDR    DDRC
#define PAR_PORT_PIN    PINC

/*
 * Use always_inline to override gcc's -Os option. Since we measured each
 * inline function's disassembly and verified the size decrease, we are
 * certain when we specify inline that we really want it.
 */
#define INLINE          static inline __attribute__((always_inline))

/*
 * Routines for getting/setting individual IEC lines and parallel port.
 *
 * We no longer add a short delay after changing line(s) state, even though
 * it takes about 0.5 us for the line to stabilize (measured with scope).
 * This is because we need to toggle SRQ quickly to send data to the 1571
 * and the delay was breaking our deadline.
 *
 * These are all inlines and this was incrementally measured that each
 * decreases the firmware size. Some (set/get) compile into a single
 * instruction (say, sbis). This works because the "line" argument is
 * almost always a constant.
 */

INLINE void
iec_set(uint8_t line)
{
    CBM_PORT |= line;
}

INLINE void
iec_release(uint8_t line)
{
    CBM_PORT &= ~line;
}

INLINE void
iec_set_release(uint8_t s, uint8_t r)
{
    CBM_PORT = (CBM_PORT & ~r) | s;
}

INLINE uint8_t
iec_get(uint8_t line)
{
    return ((CBM_PIN >> 1) & line) == 0 ? 1 : 0;
}

INLINE uint8_t
iec_pp_read(void)
{
    PAR_PORT_DDR = 0;
    PAR_PORT_PORT = 0;
    return PAR_PORT_PIN;
}

INLINE void
iec_pp_write(uint8_t val)
{
    PAR_PORT_DDR = 0xff;
    PAR_PORT_PORT = val;
}

INLINE uint8_t
iec_poll_pins(void)
{
    return CBM_PIN >> 1;
}

// Status indicators (LEDs)
uint8_t board_get_status(void);
void board_set_status(uint8_t status);
void board_update_display(void);
bool board_timer_fired(void);

#endif // _BOARD_USBKEY_H
