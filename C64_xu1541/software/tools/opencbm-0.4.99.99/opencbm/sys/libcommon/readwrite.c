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
** \file sys/libcommon/readwrite.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Perform reading from and writing to the driver
**
****************************************************************/

#include <wdm.h>
#include "cbm_driver.h"
#include "iec.h"

/*! \brief Check the buffer of an read or write IRP

 Check the buffer of an read or write IRP

 \param IrpSp: 
   Pointer to the IO_STACK_LOCATION of the IRP which contains the input buffer.

 \return 
   If the provided buffer is valid, this function returns
   STATUS_SUCCESS. If not, it returns an appropriate error value.
*/
static NTSTATUS
cbm_checkbuffer(IN PIO_STACK_LOCATION IrpSp)
{
    NTSTATUS ntStatus;

    FUNC_ENTER();

    // The following code assumes that the read and write structure are exactly
    // the same (despite the name). This ASSERT() makes sure that we are informed
    // if this is not the case.

    DBG_ASSERT(&IrpSp->Parameters.Read.ByteOffset == &IrpSp->Parameters.Write.ByteOffset);

    if ((IrpSp->Parameters.Write.ByteOffset.HighPart != 0) ||
        (IrpSp->Parameters.Write.ByteOffset.LowPart != 0)) 
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    } 
    else 
    {
        ntStatus = STATUS_SUCCESS;
    }
    FUNC_LEAVE_NTSTATUS(ntStatus);
}


/*! \brief Services reads from or writes to the driver

 Services reads from or writes to the driver

 \param Fdo
   Pointer to a DEVICE_OBJECT structure. 
   This is the device object for the target device, 
   previously created by the driver's AddDevice routine.

 \param Irp
   Pointer to an IRP structure that describes the requested I/O operation. 

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. 
   Otherwise, it return one of the error status values.

 The driver's DriverEntry routine stored this routine's address in 
 DriverObject->MajorFunction[IRP_MJ_READ] and
 DriverObject->MajorFunction[IRP_MJ_WRITE].

 Generally, all Dispatch routines execute in an arbitrary thread context 
 at IRQL PASSIVE_LEVEL, but there are exceptions.
*/
NTSTATUS
cbm_readwrite(IN PDEVICE_OBJECT Fdo, IN PIRP Irp)
{
    PIO_STACK_LOCATION irpSp;
    PDEVICE_EXTENSION pdx;
    NTSTATUS ntStatus;
    ULONG readWriteBytesProcessed;
    ULONG readWriteLength;
    PUCHAR readWriteBuffer;

    FUNC_ENTER();

    // get the device extension

    pdx = Fdo->DeviceExtension;

    // get the current IRP stack location

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    // Check if the buffer is valid

    ntStatus = cbm_checkbuffer(irpSp);

    DBG_IRPPATH_PROCESS("read/write");

    // Number of read (or written) bytes are 0 until now.
    // This is needed for finding out if we had success, or if something went wrong

    readWriteLength = 0;


    if (NT_SUCCESS(ntStatus))
    {
        // The following code assumes that the read and write structure are exactly
        // the same (despite the name). This ASSERT() makes sure that we are informed
        // if this is not the case.

        DBG_ASSERT(&irpSp->Parameters.Read.Length == &irpSp->Parameters.Write.Length);

        // Get the number of bytes to be read or written

        readWriteLength = irpSp->Parameters.Read.Length;

        // If we do performance measurements, log the appropriate event

        if (irpSp->MajorFunction == IRP_MJ_READ)
        {
            PERF_EVENT_READ_QUEUE(irpSp->Parameters.Read.Length);
        }
        else
        {
            PERF_EVENT_WRITE_QUEUE(irpSp->Parameters.Write.Length);
        }

        // If the read or write request has another length than 0,
        // then queue the IRP for being processed later.

        if (readWriteLength != 0)
        {
            // now, queue the IRP to be processed

            ntStatus = QueueStartPacket(&pdx->IrpQueue, Irp, FALSE, Fdo);
        }
    }

    if (!NT_SUCCESS(ntStatus) || readWriteLength == 0)
    {
        // there was an error, or a read/write request with length 0:
        // Thus, complete the request

        QueueCompleteIrp(NULL, Irp, ntStatus, 0);
    }

    FUNC_LEAVE_NTSTATUS(ntStatus);
}

/*! \brief Executes reads from or writes to the driver

 Services reads from or writes to the driver

 \param Pdx
   Pointer to the DEVICE_EXTENSION structure.

 \param Irp
   Pointer to an IRP structure that describes the requested I/O operation. 

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. 
   Otherwise, it return one of the error status values.

 This function does not perform any validity checks on the input and 
 output buffer! This should already been done in cbm_readwrite.
*/
NTSTATUS
cbm_execute_readwrite(IN PDEVICE_EXTENSION Pdx, IN PIRP Irp)
{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS ntStatus;
    ULONG readWriteBytesProcessed;
    ULONG readWriteLength;
    PUCHAR readWriteBuffer;

    FUNC_ENTER();

    // get the current IRP stack location

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    // The following code assumes that the read and write structure are exactly
    // the same (despite the name). This ASSERT() makes sure that we are informed
    // if this is not the case.

    DBG_ASSERT(&irpSp->Parameters.Read.Length == &irpSp->Parameters.Write.Length);

    // Find out how much bytes are to be read/written

    readWriteLength = irpSp->Parameters.Read.Length;

    // get the buffer where the bytes to be written are / where the bytes to be read
    // should be placed

    readWriteBuffer = Irp->AssociatedIrp.SystemBuffer;

    // If we do performance measurements, log the appropriate event

    if (irpSp->MajorFunction == IRP_MJ_READ)
    {
        PERF_EVENT_READ_EXECUTE(irpSp->Parameters.Read.Length);
    }
    else
    {
        PERF_EVENT_WRITE_EXECUTE(irpSp->Parameters.Write.Length);
    }

    DBG_IRPPATH_EXECUTE("read/write");

    // As this has been tested in cbm_readwrite() already, we should not get
    // any zero length here. Anyway, be sure that this does not happen with the
    // help of this ASSERT()

    DBG_ASSERT(readWriteLength != 0);

    if (readWriteLength != 0)
    {
        // Execute the appropriate function (read or write)

        switch (irpSp->MajorFunction)
        {
        case IRP_MJ_READ: 
            ntStatus = cbmiec_raw_read(Pdx, readWriteBuffer, readWriteLength,
                &readWriteBytesProcessed);
            break;

        case IRP_MJ_WRITE:
            ntStatus = cbmiec_raw_write(Pdx, readWriteBuffer, readWriteLength,
                &readWriteBytesProcessed);
            break;

        default:
            DBG_ERROR((DBG_PREFIX "UNKNOWN IRP_MJ code in cbm_readwrite!"));
            ntStatus = STATUS_INTERNAL_ERROR;
            readWriteBytesProcessed = 0;
            break;
        }
    }

    // It would not make sense to pend the IRP here. Thus, make sure this does
    // not happen at all

    DBG_ASSERT(ntStatus != STATUS_PENDING);

    // If the read or write has not been pended, we are ready the complete this
    // IRP
    //! \todo Remove this, is this does not make sense.

    if (ntStatus != STATUS_PENDING)
    {
        QueueCompleteIrp(&Pdx->IrpQueue, Irp, ntStatus, readWriteBytesProcessed);
    }

    FUNC_LEAVE_NTSTATUS(ntStatus);
}
