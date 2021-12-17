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
** \file sys/libiec/untalk.c \n
** \author Spiro Trikaliotis \n
** \authors Based on code from
**    Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
** \n
** \brief Send an UNTALK to the IEC bus
**
****************************************************************/

#include <wdm.h>
#include "cbm_driver.h"
#include "i_iec.h"

/*! \brief Send an UNTALK over the IEC bus

 This function sends an UNTALK to the IEC bus.

 \param Pdx
   Pointer to the device extension.

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. Otherwise, it
   returns one of the error status values.
*/
NTSTATUS
cbmiec_untalk(IN PDEVICE_EXTENSION Pdx)
{
    NTSTATUS ntStatus;
    ULONG sent;
    UCHAR buffer;

    FUNC_ENTER();

    // send a 0x5F (untalk) under control of ATN

    buffer = 0x5f;
    ntStatus = cbmiec_i_raw_write(Pdx, &buffer, 1, &sent, 1, 0);

    Pdx->DoNotReleaseBus = FALSE;

    FUNC_LEAVE_NTSTATUS(ntStatus);
}
