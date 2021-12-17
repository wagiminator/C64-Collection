/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2004 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Copyright 2001-2004 Spiro Trikaliotis
 *
 */

/*! ************************************************************** 
** \file sys/libiec/rawread.c \n
** \author Spiro Trikaliotis \n
** \authors Based on code from
**    Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
** \n
** \brief Read some bytes from the IEC bus
**
****************************************************************/

#include <wdm.h>
#include "cbm_driver.h"
#include "i_iec.h"

/*! \brief Read some bytes from the IEC bus

 This function reads some bytes from the IEC bus. 
 If debugging of function parameters is defined, output the
 given parameters and the returned values.

 \param Pdx
   Pointer to the device extension.

 \param Buffer
   Pointer to a buffer where the read bytes are written to.

 \param Size
   Maximum number of characters to read from the bus.

 \param Read
   Pointer to the variable which will hold the read bytes.

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. Otherwise, it
   returns one of the error status values.
*/
NTSTATUS cbmiec_raw_read(IN PDEVICE_EXTENSION Pdx, 
                         OUT PUCHAR Buffer, IN ULONG Size, 
                         OUT ULONG* Read)
{
    NTSTATUS ntStatus;

#if DBG
    USHORT i;
#endif

    FUNC_ENTER();

    FUNC_PARAM((DBG_PREFIX "Buffer = 0x%p, Size = 0x%04x", Buffer, Size));

    ntStatus = cbmiec_i_raw_read(Pdx, Buffer, Size, Read);

#if DBG
    for (i=0;i<*Read;i++)
    {
        FUNC_PARAM((DBG_PREFIX "   input %2u: 0x%02x '%c'", i, (unsigned int) Buffer[i], (UCHAR) Buffer[i]));
    }
#endif

    FUNC_LEAVE_NTSTATUS(ntStatus);
}
