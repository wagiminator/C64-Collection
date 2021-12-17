/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Copyright 2006 Wolfgang Moser (http://d81.de)
 */

#ifndef GCR_H
#define GCR_H

#define BLOCKSIZE   256
#define GCRBUFSIZE  326

#include "opencbm.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int gcr_decode(const unsigned char *gcr,   unsigned char *decoded);
extern int gcr_encode(const unsigned char *block, unsigned char *encoded);

#ifdef __cplusplus
}
#endif

#endif
