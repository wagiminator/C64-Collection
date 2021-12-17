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
** \file sys/libcommon/startio.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief StartIo function for processing the IRP queue
**
****************************************************************/

#include <wdm.h>
#include "cbm_driver.h"

/*! \brief Execute an IRP

 This function executes an IRP. Normally, it is called from as
 StartIo() routine from the QUEUE, but it can be called directly
 (for example, if FastStart is selected).

 \param Fdo:
   Pointer to the DEVICE_OBJECT of the FDO.

 \param Irp:
   Pointer to the IRP to be executed.

 \return 
   If everything works as expected, this function returns
   STATUS_SUCCESS. If not, it returns an appropriate error value.
*/
NTSTATUS
cbm_startio(IN PDEVICE_OBJECT Fdo, IN PIRP Irp)
{
    PIO_STACK_LOCATION irpSp;
    PDEVICE_EXTENSION pdx;
    NTSTATUS ntStatus;

    FUNC_ENTER();

    // get current stack location

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    // get device extension

    pdx = Fdo->DeviceExtension;

    PERF_EVENT_STARTIO(Irp);

    // Execute the appropriate IRP

    switch (irpSp->MajorFunction)
    {
        case IRP_MJ_CREATE:
            ntStatus = cbm_execute_createopen(pdx, Irp);
            break;

        case IRP_MJ_CLOSE:
            ntStatus = cbm_execute_close(pdx, Irp);
            break;

        case IRP_MJ_READ:
            /* FALL THROUGH */

        case IRP_MJ_WRITE:
            ntStatus = cbm_execute_readwrite(pdx, Irp);
            break;

        case IRP_MJ_DEVICE_CONTROL:
        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
            ntStatus = cbm_execute_devicecontrol(pdx, Irp);
            break;

        default:
            DBG_ERROR((DBG_PREFIX "THIS SHOULD NOT HAPPEN: UNKNOWN MAJORFUNCTION %08x", 
                irpSp->MajorFunction));
            DBG_ASSERT(1==0);
            ntStatus = STATUS_NOT_SUPPORTED;
    }

    FUNC_LEAVE_NTSTATUS(ntStatus);
}
