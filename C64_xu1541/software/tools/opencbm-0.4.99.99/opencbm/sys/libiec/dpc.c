/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2004 Spiro Trikaliotis
 *
 */

/*! ************************************************************** 
** \file sys/libiec/dpc.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief DPC handle for the IEC bus
**
****************************************************************/

#include <wdm.h>
#include "cbm_driver.h"
#include "i_iec.h"

#ifdef USE_DPC

/*! \brief DPC function for releasing cbmiec_wait_for_listener()

 \param Dpc
   Pointer to the KDPC

 \param Fdo
   Pointer to the FDO

 \param Irp
   Pointer to the Irp. UNUSED.

 \param Context
   Caller-supplied Context. UNUSED.

 \param Result 
   Pointer to the variable which will hold the return value.
   After return, it will contain 1 if there was an EOI, 0 otherwise.

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. Otherwise, it
   returns one of the error status values.
*/
VOID
cbmiec_dpc(IN PKDPC Dpc, IN PDEVICE_OBJECT Fdo, IN PIRP Irp, IN PVOID Context)
{
    PDEVICE_EXTENSION pdx;

    FUNC_ENTER();

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(Context);

    DBG_DPC((DBG_PREFIX "DPC called"));

    pdx = Fdo->DeviceExtension;

    // Set the event that we are ready to quit cbmiec_wait_for_listener()

    DBG_IRQL( <= DISPATCH_LEVEL);
    KeSetEvent(&pdx->EventWaitForListener, IO_SOUND_INCREMENT, FALSE);

    FUNC_LEAVE();
}

#endif // #ifdef USE_DPC
