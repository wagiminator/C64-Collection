/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2004, 2007 Spiro Trikaliotis
 *
 */

/*! ************************************************************** 
** \file sys/libcommon/init.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Common functions für initialization the WDM and NT4 driver
**
****************************************************************/

#include <wdm.h>
#include "cbm_driver.h"
#include "iec.h"

/*! The name of this device */
/*! \todo SHOULD NOT BE STATIC! */

static UNICODE_STRING ServiceKeyRegistryPath;

/*! \brief Initialize from registry

 This function initializes some driver settings from the
 appropriate registry keys.

 \param RegistryPath
   Pointer to a UNICODE_STRING containing the name of the
   registry path from which to get the information.
   If this is NULL, this function uses the same registry
   path that was given the last time this function was
   called.

 \param Pdx
   Pointer to the device extension of the device to be
   updated.

 The RegistryPath parameter can be NULL, but this is only
 allowed on a second or subsequent call.

 Pdx can be NULL, too. If it is NULL, no device-specific
 data is read at all.

 If RegistryPath is not NULL, some memory is allocated.
 This has to be freed by calling DriverCommonUninit().
*/

VOID
cbm_init_registry(IN PUNICODE_STRING RegistryPath, IN PDEVICE_EXTENSION Pdx)
{
    NTSTATUS ntStatus;

    FUNC_ENTER();

    ntStatus = STATUS_SUCCESS;

    if (RegistryPath)
    {
        // Copy the registry path to the location

        DBG_ASSERT(ServiceKeyRegistryPath.Buffer == 0);

        // Allocate memory for the registry path

        ServiceKeyRegistryPath.Buffer = ExAllocatePoolWithTag(PagedPool, 
            RegistryPath->Length, MTAG_SERVKEY);

        // Copy the registry path into the variable

        if (ServiceKeyRegistryPath.Buffer)
        {
            ServiceKeyRegistryPath.MaximumLength = 
            ServiceKeyRegistryPath.Length = RegistryPath->Length;

            RtlCopyUnicodeString(&ServiceKeyRegistryPath, RegistryPath);
        }
        else
        {
            // No memory could be allocateed, mark the
            // length of the string appropriately

            ServiceKeyRegistryPath.MaximumLength = 
            ServiceKeyRegistryPath.Length = 0;
        }
    }

    // If there is some registry path given, read from that

    if (ServiceKeyRegistryPath.Length != 0 && ServiceKeyRegistryPath.Buffer != NULL)
    {
        HANDLE hKey;

        // Open the registry for reading

        ntStatus = cbm_registry_open_for_read(&hKey, &ServiceKeyRegistryPath);

        if (NT_SUCCESS(ntStatus))
        {
            // the cable type

            ULONG iecCable = IEC_CABLETYPE_AUTO;

#if DBG
            // In debugging versions, make sure the DebugFlags
            // are read from the registry

            cbm_registry_read_ulong(hKey, L"DebugFlags", &DbgFlags);

#endif // #if DBG

            if (Pdx)
            {
                //
                // update the cable type
                //

                cbm_registry_read_ulong(hKey, L"CableType", &iecCable);

                cbmiec_set_cabletype(Pdx, iecCable);

                //
                // update if we are requested to permanently lock the parallel port
                //

                iecCable = 1; // default is: Yes, lock
                cbm_registry_read_ulong(hKey, L"PermanentlyLock", &iecCable);
                Pdx->ParallelPortLock = iecCable ? TRUE : FALSE;

            }

            // initialize the libiec library

            cbmiec_global_init(&hKey);

            // we're done with the registry

            cbm_registry_close(hKey);
        }
        else
        {
            // An error occured. 
            // In this case, initialize the libiec with defaults

            cbmiec_global_init(NULL);
        }
    }
    else
    {
        // No registry path is given.
        // In this case, initialize the libiec with defaults

        cbmiec_global_init(NULL);
    }

    FUNC_LEAVE();
}

/*! \brief Initialize the cable

 This function initializes the cable. For this, it makes
 sure the initialization is started asynchronously.

 \param Pdx
   Pointer to the DEVICE_EXTENSION (unused).
*/

VOID
cbm_initialize_cable_deferred(IN PDEVICE_EXTENSION Pdx)
{
    FUNC_ENTER();

    Pdx->CableInitTimer = 5; // wait 5 seconds before initializing the cable

    DBG_IRQL( <= DISPATCH_LEVEL);
    IoStartTimer(Pdx->Fdo);

    FUNC_LEAVE();
}


/*! \brief Free the IRP on IRP completion

 This function is the boilerplate for freeing the IRP
 which has been allocated by IoAllocateIrp() before.
 It is installed as completion routine for the IRP.

 \param Fdo
   Pointer to the DEVICE_OBJECT for which this IRP has
   been completed.

 \param Irp
   Pointer to the Irp which has completed.

 \param Context
   Pointer to the DEVICE_EXTENSION (unused).
*/

static NTSTATUS
CompleteIOCTL(PDEVICE_OBJECT Fdo, PIRP Irp, PVOID Context)
{
    NTSTATUS ntStatus;

    FUNC_ENTER();

    UNREFERENCED_PARAMETER(Context);

//    DBG_IRQL( <= DISPATCH_LEVEL);
//    IoStopTimer(Fdo);

    DBG_IRQL( <= DISPATCH_LEVEL);
    IoFreeIrp(Irp);

    FUNC_LEAVE_NTSTATUS_CONST(STATUS_MORE_PROCESSING_REQUIRED);
}

/*! \brief Initialize Cable

 This function is called as a timer routine. It's purpose is
 to initialize the cable type on driver start.

 \param Fdo
   Pointer to the DEVICE_OBJECT.

 \param Context
   Pointer to the DEVICE_EXTENSION.

 \remark
 We do not want to initialize the cable immediately on driver startup,
 as the initialization can take considerable time, and we would prevent
 the system from booting further. Thus, we delay this for some seconds
 after the driver load, to be done in our ow thread. This function
 generates an IRP for CBMCTRL_UPDATE to initialize the cable.
*/

static VOID
InitializeCableTimerRoutine(PDEVICE_OBJECT Fdo, PVOID Context)
{
    PDEVICE_EXTENSION pdx = Context;

    FUNC_ENTER();

    // InitializeCableTimerRoutine is called at DISPATCH_LEVEL
    DBG_IRQL( == DISPATCH_LEVEL);

    if (pdx->CableInitTimer > 0 && --pdx->CableInitTimer == 0)
    {
        NTSTATUS ntStatus;

        do {
            PIO_STACK_LOCATION irpSp;
            PIRP irp;

            // send a CBMCTRL_UPDATE as ioctl to myself

            DBG_IRQL( <= DISPATCH_LEVEL );
            irp = IoAllocateIrp(Fdo->StackSize, FALSE);
            if (irp == NULL)
            {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            DBG_IRQL( <= DISPATCH_LEVEL );
            irpSp = IoGetNextIrpStackLocation(irp);

            irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
            irpSp->Parameters.DeviceIoControl.IoControlCode = CBMCTRL_UPDATE;
            irpSp->Parameters.DeviceIoControl.InputBufferLength = 0;
            irpSp->Parameters.DeviceIoControl.OutputBufferLength = 0;
            irpSp->FileObject = NULL;

            DBG_IRQL( <= DISPATCH_LEVEL );
            IoSetCompletionRoutine(irp, CompleteIOCTL, pdx, TRUE, TRUE, TRUE);

            DBG_IRQL( <= DISPATCH_LEVEL );
            ntStatus = IoCallDriver(Fdo, irp);

            // If we did not succeed in calling the driver, free the IRP again

            if (!NT_SUCCESS(ntStatus))
            {
                IoFreeIrp(irp);
                break;
            }

        } while (0);
    }

    FUNC_LEAVE();
}

/*! \brief Perform driver initialization, common to WDM and NT4 driver

 This function is called from the DriverEntry() of either the WDM
 or the NT4 driver.

 \param Driverobject
   Pointer to the DRIVER_OBJECT structure given to DriverEntry().

 \param RegistryPath
   Pointer to a UNICODE_STRING containing the name of the
   registry path from which to get the information.

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. 
   Otherwise, it returns one of the error status values.
*/
NTSTATUS
DriverCommonInit(IN PDRIVER_OBJECT Driverobject, IN PUNICODE_STRING RegistryPath)
{
    FUNC_ENTER();

    // If performance evaluation is active, initialize that

    PERF_INIT();

    // Initialize the settings from the registry

    cbm_init_registry(RegistryPath, NULL);

    // set the function pointers to our driver

    Driverobject->DriverUnload = DriverUnload;
    Driverobject->MajorFunction[IRP_MJ_CREATE] = cbm_createopenclose;
    Driverobject->MajorFunction[IRP_MJ_CLOSE] = cbm_createopenclose;
    Driverobject->MajorFunction[IRP_MJ_CLEANUP] = cbm_cleanup;
    Driverobject->MajorFunction[IRP_MJ_READ] = cbm_readwrite;
    Driverobject->MajorFunction[IRP_MJ_WRITE] = cbm_readwrite;
    Driverobject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = cbm_devicecontrol;
    Driverobject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = cbm_devicecontrol;

    FUNC_LEAVE_NTSTATUS_CONST(STATUS_SUCCESS);
}

/*! \brief Undo what DriverCommonInit() has done

 This function is called from the DriverUnload() of either the WDM
 or the NT4 driver.

 This function frees memory allocated by cbm_init_registry().
*/
VOID
DriverCommonUninit(VOID)
{
    FUNC_ENTER();

    // If performance evaluation is active, stop it

    PERF_SAVE();

    // If we allocated memory for the service key registry
    // path, free that memory.

    if (ServiceKeyRegistryPath.Buffer)
    {
        ExFreePool(ServiceKeyRegistryPath.Buffer);
        DBGDO(ServiceKeyRegistryPath.Buffer = NULL; 
        ServiceKeyRegistryPath.MaximumLength = ServiceKeyRegistryPath.Length = 0);
    }

    FUNC_LEAVE();
}

/*! \brief Initialize device object, common to WDM and NT4 driver

 This function initializes the device object, as done in 
 AddDevice() of a WDM driver, or in DriverEntry() for an NT4
 driver.

 \param Fdo
   Pointer to a DEVICE_OBJECT structure. 
   This is the device object for the target device, 
   previously created by the driver.

 \param DeviceName
   Pointer to the name of the device.

 \param ParallelPortName
   Pointer to the name of the parallel port driver which this
   device will use.

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. 
   Otherwise, it return one of the error status values.

 This function performs the following steps:
 \n 1. Initialize the Pdx
 \n 2. Initialize the Queue inside of the Pdx
 \n 3. Initialize the operating mode of the Fdo 
       (Buffered, Direct, Neither)
 \n 4. Get the information from the parallel port
 \n 5. Start the Worker Thread
 \n 6. Log either success or failure
*/
NTSTATUS
AddDeviceCommonInit(IN PDEVICE_OBJECT Fdo, IN PUNICODE_STRING DeviceName, 
                    IN PCWSTR ParallelPortName)
{
    PDEVICE_EXTENSION pdx;
    UNICODE_STRING parallelPortName;
    NTSTATUS ntStatus;

    FUNC_ENTER();

    // Initialize the Device Extension

    pdx = Fdo->DeviceExtension;

    // make sure the device extension is initialized to zero

    RtlZeroMemory(pdx, sizeof(*pdx));

    // Store the name in the device extension

    pdx->DeviceName.Buffer = DeviceName->Buffer;
    pdx->DeviceName.Length = DeviceName->Length;
    pdx->DeviceName.MaximumLength = DeviceName->MaximumLength;

    // a back-pointer to the fdo contained in the extension

    pdx->Fdo = Fdo;

    // Initialize the QUEUE object

    QueueInit(&pdx->IrpQueue, cbm_startio);

    // we want to do buffered I/O
    // since we are not passing very big portions of data,
    // the speed disadvantage is not that big

    Fdo->Flags |= DO_BUFFERED_IO;

    // Generate a UNICODE_STRING containing the name of the
    // parallel port driver

    parallelPortName.Buffer = (PWSTR) ParallelPortName;
    parallelPortName.Length = (USHORT) wcslen(ParallelPortName) * sizeof(WCHAR);
    parallelPortName.MaximumLength = parallelPortName.Length;

    // Mark if we are running on an machine with more than one processor

/*
#ifdef COMPILE_W98_API
    pdx->IsSMP = 0;
#elif COMPILE_W2K_API
    pdx->IsSMP = (*KeNumberProcessors > 1) ? TRUE : FALSE;
#else
    pdx->IsSMP = (KeNumberProcessors > 1) ? TRUE : FALSE;
#endif
*/
    pdx->IsSMP = (CbmGetNumberProcessors() > 1) ? TRUE : FALSE;

    // Initialize timer so that it can be used later on for device initialization

    DBG_IRQL( == PASSIVE_LEVEL);
    IoInitializeTimer(pdx->Fdo, InitializeCableTimerRoutine, pdx);

    // Initialize the parallel port information

    ntStatus = ParPortInit(&parallelPortName, pdx);

    if (!NT_SUCCESS(ntStatus))
    {
        DBG_ERROR((DBG_PREFIX "ParPortInit() FAILED, deleting device"));
        LogErrorString(Fdo, CBM_START_FAILED, L"initialization of the parallel port.", NULL);
    }

    // Now, start the worker thread

    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = cbm_start_thread(pdx);

        cbm_initialize_cable_deferred(pdx);
    }

    // Now, log success to the event logger if we succeeded.
    // If we failed, the FDO will be deleted soon.

    if (NT_SUCCESS(ntStatus))
    {
        LogErrorOnly(Fdo, CBM_STARTED);
    }

    FUNC_LEAVE_NTSTATUS(ntStatus);
}
