/* Name: s1.c
 * Project: xu1541
 * Author: Till Harbaum
 * Tabsize: 4
 * Copyright: (c) 2007 by Till Harbaum <till@harbaum.org>
 * License: GPL
 *
 */

/* This file contains the "serial1" helper functions for opencbm */
/* changes in the protocol must be reflected here. */

#include <avr/io.h>
#include <avr/wdt.h>

#include "xu1541.h"
#include "s1.h"


static void s1_write_byte(uchar c) {
  uchar i;

  wdt_reset();

  for(i=0; i<8; i++, c<<=1) {
    if(c & 0x80) { iec_set(DATA); } else { iec_release(DATA); }
    iec_release(CLK);
    while(!iec_get(CLK));

    if(c & 0x80) { iec_release(DATA); } else { iec_set(DATA); }
    while(iec_get(CLK));

    iec_release(DATA);
    iec_set(CLK);
    while(!iec_get(DATA));
  }
}

uchar s1_write(uchar *data, uchar len) {
  uchar i;

  for(i=0;i<len;i++)
    s1_write_byte(*data++);

  return len;
}

static uchar s1_read_byte(void) {
  char i;
  uchar b, c;

  wdt_reset();

  c = 0;
  for(i=7; i>=0; i--) {
    while(iec_get(DATA));

    iec_release(CLK);
    b = iec_get(CLK);
    c = (c >> 1) | (b ? 0x80 : 0);
    iec_set(DATA);
    while(b == iec_get(CLK));

    iec_release(DATA);

    while(!iec_get(DATA));

    iec_set(CLK);
  }
  return c;
}

uchar s1_read(uchar *data, uchar len) {
  uchar i;

  for(i=0;i<len;i++)
    *data++ = s1_read_byte();

  return len;
}

