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
** \file sys/libiec/listen.c \n
** \author Spiro Trikaliotis \n
** \authors Based on code from
**    Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
** \n
** \brief Send a LISTEN to the IEC bus
**
****************************************************************/

#include <wdm.h>
#include "cbm_driver.h"
#include "i_iec.h"

/*! \brief Send a LISTEN over the IEC bus

 This function sends a listen on the IEC bus.

 \param Pdx
   Pointer to the device extension.

 \param Device
   Device (primary) address

 \param Secaddr
   Secondary address

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. Otherwise, it
   returns one of the error status values.
*/
NTSTATUS
cbmiec_listen(IN PDEVICE_EXTENSION Pdx, IN UCHAR Device, IN UCHAR Secaddr)
{
    NTSTATUS ntStatus;
    ULONG sent;
    UCHAR buffer[2];

    FUNC_ENTER();

    FUNC_PARAM((DBG_PREFIX "Device = 0x%02x, Secaddr = 0x%02x", (int)Device, (int)Secaddr));

    // send a 0x2x / 0x6y (listen device x, secaddr y) under control of ATN

    buffer[0] = 0x20 | Device;
    buffer[1] = 0x60 | Secaddr;
    ntStatus = cbmiec_i_raw_write(Pdx, buffer, 2, &sent, 1, 0);

    Pdx->DoNotReleaseBus = TRUE;

    FUNC_LEAVE_NTSTATUS(ntStatus);
}
