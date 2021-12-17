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
** \file sys/libcommon/cleanup.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Process an IRP_MJ_CLEANUP
**
****************************************************************/

#include <wdm.h>
#include "cbm_driver.h"
#include "iec.h"

/*! \brief Services IRPs containing the IRP_MJ_CLEANUP I/O function code.

 Services IRPs containing the IRP_MJ_CREATE I/O function code.

 \param Fdo
   Pointer to a DEVICE_OBJECT structure. 
   This is the device object for the target device, 
   previously created by the driver's AddDevice routine.

 \param Irp
   Pointer to an IRP structure that describes the requested I/O operation. 

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. 
   Otherwise, it return one of the error status values.

 The driver's DriverEntry routine has stored this routine's address 
 in DriverObject->MajorFunction[IRP_MJ_CLEANUP].
*/

NTSTATUS
cbm_cleanup(IN PDEVICE_OBJECT Fdo, IN PIRP Irp)
{
    PIO_STACK_LOCATION irpSp;
    PDEVICE_EXTENSION pdx;
    NTSTATUS ntStatus;

    FUNC_ENTER();

    pdx = Fdo->DeviceExtension;
    irpSp = IoGetCurrentIrpStackLocation(Irp);

    /* Let the QUEUE itself perform all relevant steps */
    QueueCleanup(&pdx->IrpQueue, irpSp->FileObject);

    /* We're done, complete the IRP */
    ntStatus = QueueCompleteIrp(NULL, Irp, STATUS_SUCCESS, 0);

    FUNC_LEAVE_NTSTATUS(ntStatus);
}
