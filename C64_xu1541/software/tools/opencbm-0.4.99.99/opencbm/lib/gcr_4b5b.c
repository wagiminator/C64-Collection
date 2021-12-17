/*
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *  Copyright 2006 Wolfgang Moser (http://d81.de)
 *
 */

/*! ************************************************************** 
** \file lib/gcr_4b5b.c \n
** \author Wolfgang Moser \n
** \n
** \brief Shared library / DLL for accessing the driver
**        Commodore GCR conversion functions
**
****************************************************************/

/*! Mark: We are in user-space (for debug.h) */
#define DBG_USERMODE

/*! The name of the executable */
#define DBG_PROGNAME "OPENCBM.DLL"

#include "debug.h"

//! mark: We are building the DLL */
#define DLL
#include "opencbm.h"

#include <stddef.h>

/*! \brief Decode GCR data

 This function decodes a buffer of 5 GCR bytes into a buffer
 of 4 plain bytes

 \param source  
   The pointer to the source buffer of 5 read-only GCR bytes

 \param dest
   The pointer to the destination buffer of 4 plain bytes

 \param sourceLength
   The size of the source buffer, it should be greater or
   equal to 5. If the size given is less than or equal to 0,
   this is taken as assertion, since it denotes unwanted
   programming behaviour.

 \param destLength
   The size of the destination buffer, it should be greater
   or equal to 4. If the size given is less than or equal
   to 0 this is taken as assertion, since it denotes
   unwanted programming behaviour.

 \return
   0 means (partial) success, -1 means failure due to
   invalid buffer pointers or a sourceLength of less than
   or equal to 0. Codes from 1 to 255 denote a bitmask of
   8 bits with each representing an illegal GCR nybble
   code, when set to one.

 Remarks:

 The function in fact can be called with buffer lengths less
 than recommended, it then does partial conversions as long
 as there is input available or as long as output buffer
 bytes are allowed to be written.

 The source and destination buffer pointers are allowed to
 overlap partially. As long as destination is less than
 source+2 or as long as destination is bigger than source+4,
 it is allowed to let the buffers overlap. Other conditions
 are taken as assertions since conversion errors will occur
 (data taken as input gets overwritten before). This
 behaviour allows to construct in-place buffer conversions.
*/

int CBMAPIDECL
gcr_5_to_4_decode(const unsigned char *source, unsigned char *dest,
                  size_t sourceLength,         size_t destLength)
{
    int rv;

    FUNC_ENTER();

    DBG_ASSERT( (source != NULL )     && (sourceLength  > 0) );
    DBG_ASSERT( (dest   != NULL )     && (destLength    > 0) );
    DBG_ASSERT( (dest < (source + 2)) || (dest > (source+4)) );

    if (sourceLength < 5)
    {
        DBG_WARN((DBG_PREFIX "*** source buffer is less than 5 bytes, only partial decoding done!"));
    }
    if (destLength < 4)
    {
        DBG_WARN((DBG_PREFIX "*** destination buffer is less than 4 bytes, only partial decoding done!"));
    }

    if( (source == NULL) || (dest == NULL) || (sourceLength<=0) )
    {
        rv = -1;
    }
    else
    {
        int i;

            /* 255 denotes illegal GCR bytes, for error checking extensions */
        static const unsigned char decodeGCR[32] =
            {255,255,255,255,255,255,255,255,255,  8,  0,  1,255, 12,  4,  5,
             255,255,  2,  3,255, 15,  6,  7,255,  9, 10, 11,255, 13, 14,255 };

            /* at least 24 bits for shifting into bits 16...20 */
        register unsigned int tdest, nybble;

        rv = 0;

        tdest   = *source;
        tdest <<= 13;

        for(i = 5; (i < 13) && (destLength > 0); i += 2, dest++, destLength--)
        {
            source++, sourceLength--;
            if(sourceLength > 0)
            {
                tdest  |= ((unsigned int)(*source)) << i;
            }
            else
            {
                    // no more input available, because GCR bytes
                    // with value 0 are not defined. But this must
                    // only be done, if at least one sourcebyte is
                    // already missing
                if ( 0 == ((tdest >> 16) & 0x3ff) ) break;
            }

                // "tdest >> 16" could be optimized to a word
                // aligned access, hopefully the compiler does
                // this for us (in a portable way)
            nybble  = decodeGCR[ (tdest >> 16) & 0x1f ];
            if(nybble > 15) rv |= 2;    // invalid GCR detected
            *dest   = nybble << 4;
            tdest <<= 5;

            nybble  = decodeGCR[ (tdest >> 16) & 0x1f ];
            if(nybble > 15) rv |= 1;    // invalid GCR detected
            *dest  |= (nybble & 0x0f);
            tdest <<= 5;

            rv    <<= 2;    // mark invalid GCR codes, make room for new
        }

    }

    FUNC_LEAVE_INT(rv);
    return rv;
}

/*! \brief Encode GCR data

 This function encodes a buffer of 4 plain bytes into a buffer
 of 5 GCR bytes

 \param source  
   The pointer to the source buffer of 4 read-only plain bytes

 \param dest
   The pointer to the destination buffer of 5 GCR bytes

 \param sourceLength
   The size of the source buffer, it should be greater or
   equal to 4. If the size given is less than or equal to 0
   this is taken as assertion, since it denotes unwanted
   programming behaviour.

 \param destLength
   The size of the destination buffer, it should be greater
   or equal to 5. If the size given is less than or equal
   to 0 this is taken as assertion, since it denotes
   unwanted programming behaviour.

 \return
   0 means (partial) success, -1 means failure due to
   invalid buffer pointers or a sourceLength of less than
   or equal to 0.

 Remarks:
 \n\n
 The function in fact can be called with buffer lengths less
 than recommended, it then does partial conversions as long
 as there is input available or as long as output buffer
 bytes are allowed to be written.
 \n\n
 The source and destination buffer pointers are allowed to
 overlap partially. As long as destination is less than or
 equal to source or as long as destination is bigger than
 source+3, it is allowed to let the buffers overlap. Other
 conditions are taken as assertions since conversion errors
 will occur (data taken as input gets overwritten before).
 This behaviour allows to construct in-place buffer
 conversions.
*/

int CBMAPIDECL
gcr_4_to_5_encode(const unsigned char *source, unsigned char *dest,
                  size_t sourceLength,         size_t destLength)
{
    int rv;

    FUNC_ENTER();

    DBG_ASSERT( (source !=   NULL ) && (sourceLength  > 0) );
    DBG_ASSERT( (dest   !=   NULL ) && (destLength    > 0) );
    DBG_ASSERT( (dest   <= source ) || (dest > (source+3)) );

    if (sourceLength < 4)
    {
        DBG_WARN((DBG_PREFIX "*** source buffer is less than 4 bytes, only partial decoding done!"));
    }
    if (destLength < 5)
    {
        DBG_WARN((DBG_PREFIX "*** destination buffer is less than 5 bytes, only partial decoding done!"));
    }

    if( (source == NULL) || (dest == NULL) || (sourceLength<=0) )
    {
        rv = -1;
    }
    else
    {
        static const unsigned char encodeGCR[16] =
            { 10, 11, 18, 19, 14, 15, 22, 23, 9, 25, 26, 27, 13, 29, 30, 21 };
        int i;
            /* at least 16 bits for overflow shifting */
        register unsigned int tdest = 0;

        rv = 0;

        for( i  = 2;
            (i < 10) && (sourceLength > 0) && (destLength > 0);
             i += 2, source++, sourceLength--, dest++, destLength--)
        {
            tdest <<= 5;  /* make room for the upper nybble */
            tdest  |= encodeGCR[ (*source) >>   4 ];

            tdest <<= 5;  /* make room for the lower nybble */
            tdest  |= encodeGCR[ (*source) & 0x0f ];

            *dest   = (unsigned char)(tdest >> i);
        }

        // (i == 10), if the loop exited normally
        if(destLength > 0)
        {
            // There're bits in the line, either because it is the
            // last one with bits from 0...7 or because the source
            // buffer got to its end and the remaining bits could
            // be written out partially.

            *dest = ((unsigned char)(tdest)) << ((10-i) & 0x07);
        }
    }

    FUNC_LEAVE_INT(rv);
    return rv;
}
