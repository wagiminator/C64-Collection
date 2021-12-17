/*
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *  Copyright 2011 Spiro Trikaliotis
 *
 */

/*! ************************************************************** 
** \file libmisc/statedebug.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Debug states in transfer functions of end-user tools
**
****************************************************************/

#include "statedebug.h"
#include "version.h"

#include <stdio.h>

volatile signed int DebugLineNumber=-1, DebugBlockCount=-1,
                    DebugByteCount=-1, DebugBitCount=-1;
volatile char * DebugFileName = "";

void DebugPrintDebugCounters(void)
{
    fprintf(stderr, "file: %s"
                      "\n\tversion: " OPENCBM_VERSION_STRING ", built: " __DATE__ " " __TIME__
                      "\n\tline=%d, blocks=%d, bytes=%d, bits=%d\n",
                      DebugFileName, DebugLineNumber,
                      DebugBlockCount, DebugByteCount,
                      DebugBitCount);
}
