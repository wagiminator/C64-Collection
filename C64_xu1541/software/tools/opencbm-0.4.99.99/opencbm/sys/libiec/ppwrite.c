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
** \file sys/libiec/ppwrite.c \n
** \author Spiro Trikaliotis \n
** \authors Based on code from
**    Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
** \n
** \brief Write a byte to the X[M|A]P1541-cable
**
****************************************************************/

#include <wdm.h>
#include "cbm_driver.h"
#include "i_iec.h"

/*! \brief Write a byte to the X[M|A]P1541 cable

 This function writes a byte to the parallel portion of
 the X[M|A]P1541 cable.

 \param Pdx
   Pointer to the device extension.

 \param Value
   Value to be written on the bus

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. Otherwise, it
   returns one of the error status values.
*/
NTSTATUS
cbmiec_pp_write(IN PDEVICE_EXTENSION Pdx, IN UCHAR Value)
{
    FUNC_ENTER();

    FUNC_PARAM((DBG_PREFIX "value = 0x%02x", (int)Value));

    if (Pdx->IecOutBits & PP_LP_BIDIR)
    {
        CBMIEC_RELEASE(PP_LP_BIDIR);
    }
    WRITE_PORT_UCHAR(PAR_PORT, Value);

    FUNC_LEAVE_NTSTATUS_CONST(STATUS_SUCCESS);
}
