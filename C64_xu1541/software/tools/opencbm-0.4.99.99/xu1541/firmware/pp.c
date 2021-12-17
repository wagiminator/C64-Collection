/* Name: pp.c
 * Project: xu1541
 * Author: Till Harbaum
 * Tabsize: 4
 * Copyright: (c) 2007 by Till Harbaum <till@harbaum.org>
 * License: GPL
 *
 */

/* This file contains the "parallel" helper functions for opencbm */
/* changes in the protocol must be reflected here. */

#include <avr/io.h>
#include <util/delay.h>

#include "xu1541.h"
#include "pp.h"

static void pp_write_2_bytes(uchar *c) {
  while(!iec_get(DATA));
  xu1541_pp_write(*c++);
  iec_release(CLK);

  while(iec_get(DATA));
  xu1541_pp_write(*c++);
  iec_set(CLK);
}

uchar pp_write(uchar *data, uchar len) {
  uchar i;

  for(i=0;i<len;i+=2) {
    pp_write_2_bytes(data);
    data += 2;
  }

  return len;
}

static void pp_read_2_bytes(uchar *c) {
  while(!iec_get(DATA));
  *c++ = xu1541_pp_read();
  iec_release(CLK);

  while(iec_get(DATA));
  *c++ = xu1541_pp_read();
  iec_set(CLK);
}

uchar pp_read(uchar *data, uchar len) {
  uchar i;

  for(i=0;i<len;i+=2) {
    pp_read_2_bytes(data);
    data += 2;
  }

  return len;
}

