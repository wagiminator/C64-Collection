/*
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *  Copyright 2005 Spiro Trikaliotis
 *
*/

/*! ************************************************************** 
** \file arch/windows/dbghelp.c \n
** \author Spiro Trikaliotis \n
** \version $Id: dbghelp.c,v 1.4 2008-10-09 17:14:26 strik Exp $ \n
** \n
** \brief Some debugging help functions
**
****************************************************************/

/*! Mark: We are in user-space (for debug.h) */
#define DBG_USERMODE

/*! The name of the executable */
#define DBG_PROGNAME "ARCH.LIB"

#include "debug.h"

#include "arch.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

// #define DBG_DUMP_RAW_READ
// #define DBG_DUMP_RAW_WRITE

/*-------------------------------------------------------------------*/
/*--------- DEBUGGING FUNCTIONS -------------------------------------*/

#if DBG

/*! \brief output a memory dump to the debugging system

 Generates a byte dump and outputs it to the debugging system

 \param Where
   Some text to output in front of the memory dump.
   This text can be used to identify where this dump
   occurred.

 \param InputBuffer
   Pointer to a buffer which contains the data
   bytes to be dumped.

 \param Count
   The size of the buffer to be dumped.

 \remark
   as dumping memory is used regularly when debugging,
   we give a debug helper function for this
*/
void 
dbg_memdump(const char *Where, const unsigned char *InputBuffer, const unsigned int Count)
{
    unsigned i;
    char outputBufferChars[17];
    char outputBuffer[100];
    char *p;

    p = outputBuffer;

    DBG_PRINT((DBG_PREFIX "%s: (0x%04x)", Where, Count));

    for (i=0; i<Count; i++) 
    {
        p += sprintf(p, "%02x ", (unsigned int) InputBuffer[i]);

        if (i % 16 == 7)
        {
            p += sprintf(p, "- ");
        }

        outputBufferChars[i % 16] = isprint(InputBuffer[i]) ? InputBuffer[i] : '.';

        if (i % 16 == 15)
        {
            outputBufferChars[(i % 16) + 1] = 0;
            DBG_PRINT((DBG_PREFIX "%04x: %-50s  %s",
                i & 0xfff0, outputBuffer, outputBufferChars));
            p = outputBuffer;
        }
    }

    if (i % 16 != 0)
    {
        outputBufferChars[i % 16] = 0;
        DBG_PRINT((DBG_PREFIX "%04x: %-50s  %s",
            i & 0xfff0, outputBuffer, outputBufferChars));
    }
}

#endif // #if DBG
