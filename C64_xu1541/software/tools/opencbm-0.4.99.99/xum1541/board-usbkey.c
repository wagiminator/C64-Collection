/*
 * Board interface routines for the USBKEY development kit
 * Copyright (c) 2009-2010 Nate Lawson <nate@root.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#include <LUFA/Drivers/Board/LEDs.h>
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

// Initialize the board (timer, indicator LEDs, UART)
void
board_init(void)
{
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

    LEDs_Init();

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
    /*
     * Configure the IEC port for 4 inputs, 4 outputs.
     *
     * Add pull-ups on all the inputs since we're running at 3.3V while
     * the 1541 is at 5V.  This causes current to flow into our Vcc
     * through the 1541's 1K ohm pull-ups but with our pull-ups also,
     * the current is only ~50 uA (versus 1.7 mA with just the 1K pull-ups).
     */
    CBM_DDR = IO_OUTPUT_MASK;
    CBM_PORT = (uint8_t)~IO_OUTPUT_MASK;
    PAR_PORT_PORT = 0;
    PAR_PORT_DDR = 0;
}

#define LED_UPPER_RED   LEDS_LED3
#define LED_UPPER_GREEN LEDS_LED4
#define LED_LOWER_RED   LEDS_LED1
#define LED_LOWER_GREEN LEDS_LED2

static uint8_t statusValue;
static uint8_t statusMask;

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
        LEDs_SetAllLEDs(LED_UPPER_RED);
        break;
    case STATUS_CONNECTING:
        LEDs_SetAllLEDs(LED_UPPER_GREEN);
        break;
    case STATUS_READY:
        LEDs_SetAllLEDs(LED_LOWER_GREEN);
        break;
    case STATUS_ACTIVE:
        // Toggle both green LEDs while busy
        statusMask = LED_UPPER_GREEN | LED_LOWER_GREEN;
        LEDs_SetAllLEDs(statusMask);
        break;
    case STATUS_ERROR:
        // Set both red on error
        statusMask = LED_UPPER_RED | LED_LOWER_RED;
        LEDs_SetAllLEDs(statusMask);
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
        LEDs_SetAllLEDs(LEDs_GetLEDs() ^ statusMask);
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
