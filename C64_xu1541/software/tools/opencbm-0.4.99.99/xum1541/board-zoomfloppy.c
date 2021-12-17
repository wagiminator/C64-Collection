/*
 * Board interface routines for the ZoomFloppy
 * Copyright (c) 2010 Nate Lawson <nate@root.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#include "xum1541.h"

#ifdef DEBUG
// Send a byte to the UART for debugging printf()
static int
uart_putchar(char c, FILE *stream)
{
    if (c == '\n')
        uart_putchar('\r', stream);
    loop_until_bit_is_set(UCSR1A, UDRE1);
    UDR1 = c;
    return 0;
}
static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
#endif // DEBUG

// Initialize the board (timer, indicator LED, UART)
void
board_init(void)
{
    // Initialize just the IO pin for LED at this point
    PORTC = 0;
    DDRC = LED_MASK;

#ifdef DEBUG
    /*
     * Initialize the UART baud rate at 115200 8N1 and select it for
     * printf() output.
     */
    UCSR1A |= _BV(U2X1);
    UCSR1B |= _BV(TXEN1);
    UBRR1 = 8;
    stdout = &mystdout;
#endif

    // Setup 16 bit timer as normal counter with prescaler F_CPU/1024.
    // We use this to create a repeating 100 ms (10 hz) clock.
    OCR1A = (F_CPU / 1024) / 10;
    TCCR1B |= (1 << WGM12) | (1 << CS02) | (1 << CS00);
}

// Initialize the board IO ports for IEC mode
// This function has to work even if the ports were left in an indeterminate
// state by a prior initialization (e.g., auto-probe for IEEE devices).
void
board_init_iec(void)
{
    // IO port initialization. CBM is on D and C, parallel B.
    PORTC = 0;
    PORTD = 0;
    DDRC = IO_MASK_C;
    DDRD = IO_MASK_D;
    PAR_PORT_PORT = 0;
    PAR_PORT_DDR = 0;
}

uint8_t
iec_poll_pins(void)
{
    return ((PIND & IO_SRQ_IN)  >> 1) |
           ((PIND & IO_CLK_IN)  >> 1) |
           ((PIND & IO_DATA_IN) << 1) |
           ((PINC & IO_ATN_IN)  << 1) |
           ((PINC & IO_RESET_IN) >> 1);
}

static uint8_t statusValue;

uint8_t
board_get_status()
{
    return statusValue;
}

// Status indicators (LEDs for this board)
void
board_set_status(uint8_t status)
{
    statusValue = status;

    switch (status) {
    case STATUS_INIT:
        PORTC |= LED_MASK;
        break;
    case STATUS_CONNECTING:
        break;
    case STATUS_READY:
        // Turn off LED
        PORTC &= ~LED_MASK;
        break;
    case STATUS_ACTIVE:
        // Turn on LED. The update routine will toggle it.
        PORTC |= LED_MASK;
        break;
    case STATUS_ERROR:
        // Set red on error
        PORTC |= LED_MASK;
        break;
    default:
        DEBUGF(DBG_ERROR, "badstsval %d\n", status);
    }
}

/*
 * Callback for when the timer fires.
 * Update LEDs or do other tasks that should be done about every
 */
void
board_update_display()
{
    if (statusValue == STATUS_ACTIVE || statusValue == STATUS_ERROR)
        PORTC ^= LED_MASK;
}

/* 
 * Signal that the board_update_display() should be called if the timer
 * has fired (every ~100 ms).
 */
bool
board_timer_fired()
{
    // If timer fired, clear overflow bit and notify caller.
    if ((TIFR1 & (1 << OCF1A)) != 0) {
        TIFR1 |= (1 << OCF1A);
        return true;
    } else
        return false;
}
