/* Name: s2.c
 * Project: xu1541
 * Author: Till Harbaum
 * Tabsize: 4
 * Copyright: (c) 2007 by Till Harbaum <till@harbaum.org>
 * License: GPL
 *
 */

/* This file contains the "serial2" helper functions for opencbm */
/* changes in the protocol must be reflected here. */

#include <avr/io.h>
#include <avr/wdt.h>

#include "xu1541.h"
#include "s2.h"

static void s2_write_byte(uchar c) {
  uchar i;

  wdt_reset();

  for(i=0; i<4; i++) {
    if(c & 1) { iec_set(DATA); } else { iec_release(DATA); }
    c >>= 1;
    iec_release(ATN);
    while(iec_get(CLK));

    if(c & 1) { iec_set(DATA); } else { iec_release(DATA); }
    c >>= 1;
    iec_set(ATN);

    while(!iec_get(CLK));
  }

  iec_release(DATA);
}

uchar s2_write(uchar *data, uchar len) {
  uchar i;

  for(i=0;i<len;i++)
    s2_write_byte(*data++);

  return len;
}

static uchar s2_read_byte(void) {
  uchar c = 0;
  char i;

  wdt_reset();

  for(i=4; i>0; i--) {
    while(iec_get(CLK));

    c = (c>>1) | (iec_get(DATA) ? 0x80 : 0);
    iec_release(ATN);
    while(!iec_get(CLK));

    c = (c>>1) | (iec_get(DATA) ? 0x80 : 0);
    iec_set(ATN);
  }
  return c;
}

uchar s2_read(uchar *data, uchar len) {
  uchar i;

  for(i=0;i<len;i++)
    *data++ = s2_read_byte();

  return len;
}

