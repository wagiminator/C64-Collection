/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2004 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Copyright 2001-2004,2008-2009 Spiro Trikaliotis
 *
 */

/*! ************************************************************** 
** \file sys/libiec/ppread.c \n
** \author Spiro Trikaliotis \n
** \authors Based on code from
**    Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
** \n
** \brief Read a byte from the X[M|A]P1541-cable
**
****************************************************************/

#include <wdm.h>
#include "cbm_driver.h"
#include "i_iec.h"


/*! \brief Read a byte from the X[M|A]P1541 cable. Make sure to debounce it.

 This function reads a byte from the parallel portion of
 the X[M|A]P1541 cable. The lines are debounced.

 \param Pdx
   Pointer to the device extension.

 \return
   The read value, or -1 if debouncing failed.
*/
LONG
cbmiec_i_pp_read_debounced(IN PDEVICE_EXTENSION Pdx)
{
    int returnValue=-1, returnValue2, returnValue3, timeoutCount=0;

    FUNC_ENTER();

    returnValue3 = READ_PORT_UCHAR(PAR_PORT);
    returnValue2 = ~returnValue3;    // ensure to read once more

    do {
        if (++timeoutCount >= 8)
        {
            DBG_PRINT((DBG_PREFIX "Triple-Debounce TIMEOUT: 0x%02x, 0x%02x, 0x%02x (%d)",
                returnValue, returnValue2, returnValue3, timeoutCount));
            break;
        }
        returnValue  = returnValue2;
        returnValue2 = returnValue3;
        returnValue3 = READ_PORT_UCHAR(PAR_PORT);
    } while ((returnValue != returnValue2) || (returnValue != returnValue3));

    FUNC_LEAVE_LONG(returnValue);
}

/*! \brief Read a byte from the X[M|A]P1541 cable

 This function reads a byte from the parallel portion of
 the X[M|A]P1541 cable.

 \param Pdx
   Pointer to the device extension.

 \param Return
   Pointer to an UCHAR where the read byte is written to.

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. Otherwise, it
   returns one of the error status values.
*/
NTSTATUS
cbmiec_pp_read(IN PDEVICE_EXTENSION Pdx, OUT UCHAR *Return)
{
    LONG returnValue = -1;
    NTSTATUS ntStatus = STATUS_NO_DATA_DETECTED;

    FUNC_ENTER();

    if (!(Pdx->IecOutBits & PP_LP_BIDIR))
    {
        WRITE_PORT_UCHAR(PAR_PORT, 0xFF);
        CBMIEC_SET(PP_LP_BIDIR);
    }

    returnValue = cbmiec_i_pp_read_debounced(Pdx);

    if (returnValue >= 0)
    {
        *Return = (UCHAR) returnValue;
        ntStatus = STATUS_SUCCESS;
    }

    FUNC_LEAVE_NTSTATUS(ntStatus);
}
