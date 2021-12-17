/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2004-2006 Spiro Trikaliotis
 *
 */

/*! ************************************************************** 
** \file sys/libcommon/PortAccess.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Functions for communicating with the parallel port driver
**
****************************************************************/

#include <initguid.h>
#include <wdm.h>
#include "cbm_driver.h"

/*! \internal \brief Send an IOCTL to the parallel port driver

 This function sends an IOCTL to the parallel port driver.
 It is an internal helper function for the following functions.

 \param Pdx
   Pointer to the device extension of the driver

 \param Ioctl
   The IOCTL to perform

 \param InBuffer
   Pointer to a buffer which holds the input.
   This can be NULL.

 \param InBufferLength
   Length of the buffer pointed to by InBuffer. 
   Must be 0 if InBuffer == NULL.

 \param OutBuffer
   Pointer to a buffer which will get the output.
   This can be NULL.

 \param OutBufferLength
   Length of the buffer pointed to by OutBuffer. 
   Must be 0 if OutBuffer == NULL.

 This function must be run at IRQL == PASSIVE_LEVEL.
*/
static NTSTATUS
ParPortIoctlInOut(IN PDEVICE_EXTENSION Pdx, IN ULONG Ioctl, 
                  IN PVOID InBuffer, IN ULONG InBufferLength, 
                  OUT PVOID OutBuffer, IN ULONG OutBufferLength)
{
    IO_STATUS_BLOCK ioStatusBlock;
    NTSTATUS ntStatus;
    KEVENT event;   // event to be signalled when the IOCTL has finished
    PIRP irp;

    FUNC_ENTER();

    // Initialize the event which we will use to be notified
    // when the IRP has finished

    DBG_IRQL( == PASSIVE_LEVEL);
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    ntStatus = STATUS_SUCCESS;

    // build an IRP for this IOCTL

    DBG_IRQL( == PASSIVE_LEVEL);
    irp = IoBuildDeviceIoControlRequest(
        Ioctl,
        Pdx->ParallelPortFdo,
        InBuffer,
        InBufferLength,
        OutBuffer,
        OutBufferLength,
        TRUE,           // it's an internal device control
        &event,
        &ioStatusBlock
        );

    if (irp)
    {
        PIO_STACK_LOCATION irpStack;

        // get the current IRP stack location

        DBG_IRQL( <= DISPATCH_LEVEL);
        irpStack = IoGetNextIrpStackLocation(irp);

        // Reference the file object we are about to call.
        // This ensures the driver is not removed while we call it,
        // even if the underlying hardware is removed

        DBG_IRQL( <= DISPATCH_LEVEL);
        ObReferenceObject(Pdx->ParallelPortFileObject);

        // tell the IRP stack location to which file object we are
        // referring

        irpStack->FileObject = Pdx->ParallelPortFileObject;

        // Call the driver to perform the requested IOCTL

        DBG_IRQL( <= DISPATCH_LEVEL);
        ntStatus = IoCallDriver(Pdx->ParallelPortFdo, irp);

        // We're done, we can dereference the file object again

        DBG_IRQL( <= DISPATCH_LEVEL);
        ObDereferenceObject(Pdx->ParallelPortFileObject);

        if (!NT_SUCCESS(ntStatus))
        {
            DBG_WARN((DBG_PREFIX "IoCallDriver FAILED!"));
        }
        else 
        {
            // wait for the IRP to be completed

            DBG_IRQL( <= DISPATCH_LEVEL /* = only if timeout of NULL */);
            ntStatus = KeWaitForSingleObject(
               &event, 
               Executive, 
               KernelMode, 
               FALSE, // we are not alertable
               NULL);

            if (!NT_SUCCESS(ntStatus)) 
            {
                DBG_WARN((DBG_PREFIX "KeWaitForSingleObject FAILED!"));
            }
        }
    }

    FUNC_LEAVE_NTSTATUS(ntStatus);
}

/*! \brief Allocate a parallel port for using it

 This function allocates a parallel port, preventing other
 drivers from accessing it.

 \param Pdx
   Pointer to a device extension which contains the DEVICE_OBJECT 
   of the parallel port driver.

 This function has to be balanced with a corresponding ParPortFree()
 This function must be run at IRQL == PASSIVE_LEVEL.
*/
NTSTATUS
ParPortAllocate(PDEVICE_EXTENSION Pdx)
{
    NTSTATUS ntStatus;

    FUNC_ENTER();

    DBG_ASSERT(Pdx);
    DBG_ASSERT(Pdx->ParallelPortAllocated == FALSE);

    // allocate the parallel port

    ntStatus = ParPortIoctlInOut(Pdx, IOCTL_INTERNAL_PARALLEL_PORT_ALLOCATE,
                                 NULL, 0, NULL, 0);

    // if we were successfull, remember this in the pdx

    if (NT_SUCCESS(ntStatus))
    {
        Pdx->ParallelPortAllocated = TRUE;
    }

    FUNC_LEAVE_NTSTATUS(ntStatus);
}

/*! \brief Free a parallel port after using it

 This function frees a previously allocated parallel port.

 \param Pdx
   Pointer to a device extension which contains the DEVICE_OBJECT 
   of the parallel port driver.

 If the parallel port has not been already allocated, 
 this function just returns.

 This function must be run at IRQL == PASSIVE_LEVEL.
*/
NTSTATUS
ParPortFree(PDEVICE_EXTENSION Pdx)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    FUNC_ENTER();

    DBG_IRQL( == PASSIVE_LEVEL);

    DBG_ASSERT(Pdx != NULL);

    if (Pdx->ParallelPortAllocated == TRUE)
    {
        // Free the parallel port. An old implementation (v0.03) used
        // IOCTL_INTERNAL_PARALLEL_PORT_FREE. But this is available only
        // on WDM, that is, W2000, WXP, and above. In fact, MS specifically
        // discourages using that IOCTL, and tells us to use FreePort() 
        // instead.
        // Anyway, since I don't know what Microsoft will think in the future,
        // I decided to keep the old implementation, too.

#if 1
        Pdx->PortInfo->FreePort(Pdx->PortInfo->Context);
#else
        #error THIS IS ONLY AVAILABLE ON WDM! Thus, don't use it! (see comment above)
        ntStatus = ParPortIoctlInOut(Pdx, IOCTL_INTERNAL_PARALLEL_PORT_FREE,
                                     NULL, 0, NULL, 0);
#endif

        Pdx->ParallelPortAllocated = FALSE;
    }

    FUNC_LEAVE_NTSTATUS(ntStatus);

}

/*! \brief Initialize the knowledge on a parallel port

 This function gets some knowledge on a parallel port,
 and stores this info into the given DEVICE_EXTENSION.

 \param ParallelPortName
   UNICODE_STRING which holds the name of the parallel port driver

 \param Pdx
   Device extension which will be initialized with the needed
   knowledge on the parallel port.

 This function should be called before any other parallel port
 function is called. Usually, it is done in the driver's 
 AddDevice (WDM) or DriverEntry (WKM, WDM) function.

 One of the purposes of this function is to make sure the
 parallel port driver is not unloaded from memory (via 
 IoGetDeviceObjectPointer()).

 This function must be run at IRQL == PASSIVE_LEVEL.
*/
NTSTATUS
ParPortInit(PUNICODE_STRING ParallelPortName, PDEVICE_EXTENSION Pdx)
{
    NTSTATUS ntStatus;

    FUNC_ENTER();

    DBG_ASSERT(ParallelPortName);
    DBG_ASSERT(Pdx);

    // First of all, get the PDEVICE_OBJECT of the parallel port driver

    DBG_IRQL( == PASSIVE_LEVEL);
    ntStatus = IoGetDeviceObjectPointer(ParallelPortName, 
                                        FILE_READ_ATTRIBUTES,
                                        &Pdx->ParallelPortFileObject,
                                        &Pdx->ParallelPortFdo);

    if (!NT_SUCCESS(ntStatus))
    {
        DBG_WARN((DBG_PREFIX "IoGetDeviceObjectPointer() FAILED!"));
        FUNC_LEAVE_NTSTATUS(ntStatus);
    }

    // Allocate memory to hold to parallel port info

    DBG_IRQL( == PASSIVE_LEVEL);
    Pdx->PortInfo = (PPARALLEL_PORT_INFORMATION) ExAllocatePoolWithTag(NonPagedPool, 
        sizeof(*Pdx->PortInfo), MTAG_PPINFO);

    // If we got memory, get the info out of the parallel port driver

    if (Pdx->PortInfo)
    {
        ntStatus = ParPortIoctlInOut(Pdx, IOCTL_INTERNAL_GET_PARALLEL_PORT_INFO,
                                     NULL, 0,
                                     Pdx->PortInfo, sizeof(*Pdx->PortInfo));
    }
    else
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(ntStatus))
    {
        Pdx->ParPortPortAddress = Pdx->PortInfo->Controller;
        DBG_PPORT((DBG_PREFIX "Got parallel port information:"));
        DBG_PPORT((DBG_PREFIX "- OriginalController = 0x%p", Pdx->PortInfo->OriginalController));
        DBG_PPORT((DBG_PREFIX "- Controller         = 0x%p", Pdx->PortInfo->Controller));
        DBG_PPORT((DBG_PREFIX "- Span of controller = 0x%08x", Pdx->PortInfo->SpanOfController));
        DBG_PPORT((DBG_PREFIX "- TryAllocatePort    = 0x%p", Pdx->PortInfo->TryAllocatePort));
        DBG_PPORT((DBG_PREFIX "- FreePort           = 0x%p", Pdx->PortInfo->FreePort));
        DBG_PPORT((DBG_PREFIX "- QueryNumWaiters    = 0x%p", Pdx->PortInfo->QueryNumWaiters));
        DBG_PPORT((DBG_PREFIX "- Context            = 0x%p", Pdx->PortInfo->Context));
    }

    // if we failed getting the parallel port info, but there was memory
    // allocated, free the memory.

    if (!NT_SUCCESS(ntStatus) && Pdx->PortInfo)
    {
        DBG_IRQL( < DISPATCH_LEVEL);
        ExFreePool(Pdx->PortInfo);
        Pdx->PortInfo = NULL;
    }

    FUNC_LEAVE_NTSTATUS(ntStatus);
}

/*! \brief Undoes anything ParPortInit has done

 This function undoes anything ParPortInit() has done.

 \param Pdx
   Device extension which will be initialized with the needed
   knowledge on the parallel port.

 This function should be called as part of the unloading process
 of the driver

 One of the purposes of this function is to allow the 
 parallel port driver to be unloaded from memory (via 
 calling ObDereferenceObject()).

 This function must be run at IRQL <= DISPATCH_LEVEL.
*/
NTSTATUS
ParPortDeinit(PDEVICE_EXTENSION Pdx)
{
    FUNC_ENTER();

    DBG_IRQL( <= DISPATCH_LEVEL);

    // If the file object was previously referenced
    // (to make sure the parallel port driver is not
    // removed while we access it), dereference it now.

    if (Pdx->ParallelPortFileObject)
    {
        DBG_IRQL( <= DISPATCH_LEVEL);
        ObDereferenceObject(Pdx->ParallelPortFileObject);
        Pdx->ParallelPortFileObject = NULL;
    }

    // If we allocated memory for the parallel port info,
    // free that now.
    if (Pdx->PortInfo)
    {
        DBG_IRQL( < DISPATCH_LEVEL);
        ExFreePool(Pdx->PortInfo);
        Pdx->PortInfo = NULL;
    }

    FUNC_LEAVE_NTSTATUS_CONST(STATUS_SUCCESS);
}

/*! Verbose output of parallel port parameters */
#define DBG_PPORT_VERBOSE( _yy, _xxx, _type) \
            DBG_PPORT((DBG_PREFIX " --- " #_xxx " = " _type, PnpInfo->_xxx))

/*! \internal \brief Get the PNP, ECP and EPP information of the parallel port

 \param Pdx
   Pointer to a device extension which contains the DEVICE_OBJECT 
   of the parallel port driver.

 \param PnpInfo
   Pointer to a PARALLEL_PNP_INFORMATION structure which will contain
   the result of the function

 \return
   If the PNP information could be read, this function returns TRUE.
   \nIf it could not be read, it returns FALSE.

 This function must be run at IRQL == PASSIVE_LEVEL.
*/
static BOOLEAN
ParPortGetPnpInformation(PDEVICE_EXTENSION Pdx, PPARALLEL_PNP_INFORMATION PnpInfo)
{
    NTSTATUS ntStatus;
    BOOLEAN success;

    FUNC_ENTER();

    // get the PNP, ECP and EPP info out of the parallel port

    ntStatus = ParPortIoctlInOut(Pdx, IOCTL_INTERNAL_GET_PARALLEL_PNP_INFO,
        NULL, 0, PnpInfo, sizeof(*PnpInfo));

    if (!NT_SUCCESS(ntStatus))
    {
        // It is no problem if this fails. It only means we do not have to worry
        // about ECP and EPP modes

        DBG_WARN((DBG_PREFIX "IOCTL_INTERNAL_GET_PARALLEL_PNP_INFO FAILED with %s", DebugNtStatus(ntStatus)));

        success = FALSE;
    }
    else
    {
        success = TRUE;

        //
        // remember the ECP port address
        //

        Pdx->ParPortEcpPortAddress = PnpInfo->EcpController;

        // Output some diagnostics for debugging purposes:

        DBG_PPORT((DBG_PREFIX "IOCTL_INTERNAL_GET_PARALLEL_PNP_INFO:"));
        DBG_PPORT((DBG_PREFIX "EcpController = 0x%p", PnpInfo->EcpController));

        DBG_PPORT_VERBOSE(PHYSICAL_ADDRESS, OriginalEcpController, "0x%p");
        DBG_PPORT_VERBOSE(PUCHAR, EcpController, "0x%p");
        DBG_PPORT_VERBOSE(ULONG, SpanOfEcpController, "%u");
        DBG_PPORT_VERBOSE(ULONG, PortNumber, "%u (deprecated, do not use!)");
        DBG_PPORT_VERBOSE(ULONG, HardwareCapabilities, "%u");
        DBG_PPORT_VERBOSE(PPARALLEL_SET_CHIP_MODE, TrySetChipMode, "0x%p");
        DBG_PPORT_VERBOSE(PPARALLEL_CLEAR_CHIP_MODE, ClearChipMode, "0x%p");
        DBG_PPORT_VERBOSE(ULONG, FifoDepth, "%u");
        DBG_PPORT_VERBOSE(ULONG, FifoWidth, "%u");
        DBG_PPORT_VERBOSE(PHYSICAL_ADDRESS, EppControllerPhysicalAddress, "0x%p");
        DBG_PPORT_VERBOSE(ULONG, SpanOfEppController, "%u");
        DBG_PPORT_VERBOSE(ULONG, Ieee1284_3DeviceCount, "%u");
        DBG_PPORT_VERBOSE(PPARALLEL_TRY_SELECT_ROUTINE, TrySelectDevice, "0x%p");
        DBG_PPORT_VERBOSE(PPARALLEL_DESELECT_ROUTINE, DeselectDevice, "0x%p");
        DBG_PPORT_VERBOSE(PVOID, Context, "0x%p");
        DBG_PPORT_VERBOSE(ULONG, CurrentMode, "0x%04x");
        DBG_PPORT_VERBOSE(PWSTR, PortName, "%ws")
    }

    FUNC_LEAVE_BOOLEAN(success);
}
#undef DBG_PPORT_VERBOSE

/*! \brief Set the operational mode of the parallel port, WDM Version

 This function sets the operational mode of the parallel port.

 \param Pdx
   Pointer to a device extension which contains the DEVICE_OBJECT 
   of the parallel port driver.

 This function has to be balanced with a corresponding ParPortUnsetModeWdm()

 This function must be run at IRQL == PASSIVE_LEVEL.
*/
NTSTATUS
ParPortSetModeWdm(PDEVICE_EXTENSION Pdx)
{
    PARALLEL_PNP_INFORMATION pnpInformation;
    NTSTATUS ntStatus;

    FUNC_ENTER();

    // do not assume success for the following

    ntStatus = STATUS_INVALID_PARAMETER;

    if (ParPortGetPnpInformation(Pdx, &pnpInformation))
    {
#if DBG
        PUCHAR ecr;

        if (pnpInformation.EcpController != 0)
        {
            ecr = pnpInformation.EcpController + ECR_OFFSET;

            DBG_PPORT((DBG_PREFIX "We're having an ECP controller: ECR = %08x", ecr));
            DBG_PPORT((DBG_PREFIX " --- ECR = %02x", READ_PORT_UCHAR(ecr)));
        }
#endif

        // now, we want to set the parallel port mode. This does only
        // make sense if we have functions for setting and clearing the
        // chip mode

        if (pnpInformation.TrySetChipMode && pnpInformation.ClearChipMode)
        {
            // Does our parallel port have the capability to be set into
            // byte mode? If not, it does not make sense to change the mode

            if (pnpInformation.HardwareCapabilities & PPT_BYTE_PRESENT)
            {
                DBG_PPORT((DBG_PREFIX "Trying to set Chip mode to BYTE MODE..."));

                // try to set the chip mode to byte mode

                DBG_IRQL( <= DISPATCH_LEVEL);
                ntStatus = pnpInformation.TrySetChipMode(pnpInformation.Context, ECR_BYTE_MODE);

                DBG_PPORT((DBG_PREFIX " --- TrySetChipMode returned with %s", DebugNtStatus(ntStatus)));
            }
            else
            {
                // no byte mode available. Thus, we want to make sure we have at
                // least SPP byte.

                DBG_PPORT((DBG_PREFIX "****************************** NO BYTE MODE PRESENT, making sure to have SPP mode!!!"));
                ParPortUnsetMode(Pdx);

                // Anyway, we don't want to fail this call just because the user
                // does not have an EPP or ECP port:

                ntStatus = STATUS_SUCCESS;
            }
#if DBG
            if (pnpInformation.EcpController != 0)
            {
                DBG_PPORT((DBG_PREFIX " --- ECR = %02x", READ_PORT_UCHAR(ecr)));
            }
#endif
        }
        else
        {
            DBG_PPORT((DBG_PREFIX " --- TrySetChipMode or ClearChipmode not available!"));
        }
    }

    FUNC_LEAVE_NTSTATUS(ntStatus);
}

/*! \brief Unset the operational mode of the parallel port, WDM Version

 This function unsets the operational mode of the parallel port.

 \param Pdx
   Pointer to a device extension which contains the DEVICE_OBJECT 
   of the parallel port driver.

 This function mustn't be called without a prior call to
 ParPortSetModeWdm()

 This function must be run at IRQL == PASSIVE_LEVEL.
*/
NTSTATUS
ParPortUnsetModeWdm(PDEVICE_EXTENSION Pdx)
{
    PARALLEL_PNP_INFORMATION pnpInformation;
    NTSTATUS ntStatus;

    FUNC_ENTER();

    // assume we have success in the following

    ntStatus = STATUS_SUCCESS;

    if (ParPortGetPnpInformation(Pdx, &pnpInformation))
    {
#if DBG
        PUCHAR ecr;

        if (pnpInformation.EcpController != 0)
        {
            ecr = pnpInformation.EcpController + ECR_OFFSET;

            DBG_PPORT((DBG_PREFIX "We're having an ECP controller: ECR = %08x", ecr));

            DBG_PPORT((DBG_PREFIX " --- ECR = %02x", READ_PORT_UCHAR(ecr)));
        }
#endif

        // If we have the ClearChipMode function, try to unset the last set mode

        if (pnpInformation.ClearChipMode)
        {
            DBG_PPORT((DBG_PREFIX "Trying to unset Chip mode..."));

            DBG_IRQL( <= DISPATCH_LEVEL);
            ntStatus = pnpInformation.ClearChipMode(
                pnpInformation.Context, 
                (UCHAR) pnpInformation.CurrentMode);

            DBG_PPORT((DBG_PREFIX " --- ClearChipMode returned with %s", DebugNtStatus(ntStatus)));

            // If we did not have success, this is no problem.
            // This means that we did not set the mode in the first place,
            // thus, ignore that error.

            if (ntStatus == STATUS_UNSUCCESSFUL)
            {
                ntStatus = STATUS_SUCCESS;
            }
        }

#if DBG
        if (pnpInformation.EcpController != 0)
        {
            DBG_PPORT((DBG_PREFIX " --- ECR = %02x", READ_PORT_UCHAR(ecr)));
        }
#endif
    }

    FUNC_LEAVE_NTSTATUS(ntStatus);
}

/*! \brief Allocate an interrupt routine for a parallel port

 This function allocates an interrupt service routine for a parallel port.

 \param Pdx
   Pointer to a device extension which contains the DEVICE_OBJECT 
   of the parallel port driver.
   
 \param Isr
  Pointer to the interrupt service routine (ISR) which the caller
  wants to be installed.

 This function has to be balanced with a corresponding ParPortFreeInterrupt()

 The parallel port has to be already allocated!

 This function must be run at IRQL == PASSIVE_LEVEL.
*/
NTSTATUS
ParPortAllocInterrupt(PDEVICE_EXTENSION Pdx, PKSERVICE_ROUTINE Isr)
{
    NTSTATUS ntStatus;

    FUNC_ENTER();

    DBG_IRQL( == PASSIVE_LEVEL);

    DBG_ASSERT(Pdx);
    DBG_ASSERT(Isr);
    DBG_ASSERT(Pdx->ParallelPortAllocatedInterrupt == FALSE);

    Pdx->Pisr.InterruptServiceRoutine = Isr;
    Pdx->Pisr.InterruptServiceContext = Pdx;
    Pdx->Pisr.DeferredPortCheckRoutine = NULL;
    Pdx->Pisr.DeferredPortCheckContext = NULL;

    // try to allocate the interrupt via the IOCTL

    ntStatus = ParPortIoctlInOut(Pdx, IOCTL_INTERNAL_PARALLEL_CONNECT_INTERRUPT,
                                 &Pdx->Pisr, sizeof(Pdx->Pisr),
                                 &Pdx->Pii, sizeof(Pdx->Pii));

    // If we had success, remember this in the Pdx.

    if (NT_SUCCESS(ntStatus))
    {
        Pdx->ParallelPortAllocatedInterrupt = TRUE;
    }
    else
    {
        DBG_WARN((DBG_PREFIX "Allocation of Interrupt FAILED!"));
        LogErrorOnly(Pdx->Fdo, CBM_NO_ISR);
    }

    FUNC_LEAVE_NTSTATUS(ntStatus);
}

/*! \brief Free an interrupt routine for a parallel port after using it

 This function frees a previously allocated parallel port.

 \param Pdx
   Pointer to a device extension which contains the DEVICE_OBJECT 
   of the parallel port driver.

 If the parallel port interrupt has not been already allocated, 
 this function just returns.

 This function must be run at IRQL == PASSIVE_LEVEL.
*/
NTSTATUS
ParPortFreeInterrupt(PDEVICE_EXTENSION Pdx)
{
    NTSTATUS ntStatus;

    FUNC_ENTER();

    DBG_IRQL( == PASSIVE_LEVEL);

    DBG_ASSERT(Pdx);

    // If the interrupt was previously allocated, free that

    if (Pdx->ParallelPortAllocatedInterrupt == TRUE)
    {
        ntStatus = ParPortIoctlInOut(Pdx, IOCTL_INTERNAL_PARALLEL_DISCONNECT_INTERRUPT,
                                     &Pdx->Pisr, sizeof(Pdx->Pisr),
                                     NULL, 0);

        Pdx->ParallelPortAllocatedInterrupt = FALSE;
    }
    else
    {
        // If we did not have any interrupt, this function was successfull,
        // thus, report this as return value.

        ntStatus = STATUS_SUCCESS;
    }

    FUNC_LEAVE_NTSTATUS(ntStatus);
}

/*! \brief Set registry key such that we can get the interrupt of a parallel port

 This function sets some specific registry key which allows us to allocate an 
 interrupt service routine for a parallel port. Without this key, allocating
 the interrupt is forbidden for Win 2000, XP, and above.

 This is not true for NT4.

 \param Pdx
   Pointer to a device extension which contains the DEVICE_OBJECT 
   of the parallel port driver.

 This function must be run at IRQL == PASSIVE_LEVEL.

 \warning
  This function cannot be used on NT4! In fact, it is unnecessary there.
*/
NTSTATUS
ParPortAllowInterruptIoctl(PDEVICE_EXTENSION Pdx)
{
    PDEVICE_OBJECT pdo;
    NTSTATUS ntStatus;
    HANDLE handleReg;
    ULONG OldStateEnableConnectInterruptIoctl;
    ULONG OldStateFilterResourceMethod;

    FUNC_ENTER();

    DBG_ASSERT(Pdx);

    // open the hardware key to the parallel port

    ntStatus = cbm_registry_open_hardwarekey(&handleReg, &pdo, Pdx);

    if (NT_SUCCESS(ntStatus))
    {
        // We could open the hardware key. Now, we get the old values,
        // and set the values we need.

        // EnableConnectInterruptIoctl allows us to issue the
        // IOCTL, or it blocks us. If this registry key is not set,
        // the parport driver will not even consider to let us obtain
        // the interrupt.
        // The possible values are:
        // - 0: The IOCTL is forbidden (default)
        // - else: The IOCTL is allowed
        // This value is tested with every call to the IOCTL

        ntStatus = cbm_registry_read_ulong(handleReg, L"EnableConnectInterruptIoctl",
            &OldStateEnableConnectInterruptIoctl);

        // If the value was not there, remember it in the OldState var

        if (ntStatus == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            OldStateEnableConnectInterruptIoctl = -1;
            ntStatus = STATUS_SUCCESS;
        }

        // FilterResourceMethod tells the parallel port if it should try
        // to get the interrupt itself in the first place. If the parallel port
        // does not have an interrupt, we cannot obtain it ourselves.
        // The possible values are:
        // - 0: Try to not obtain the interrupt, if such a hardware
        //      configuration exists
        // - 1: Force not to use the interrupt (default)
        // - 2: Always use the interrupt
        // Unfortunately, this entry is only checked on device initialization.
        // Because of this, the parport needs to be restarted for this change
        // to take effect.

        ntStatus = cbm_registry_read_ulong(handleReg, L"FilterResourceMethod",
            &OldStateFilterResourceMethod);

        // If the value was not there, remember it in the OldState var

        if (ntStatus == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            OldStateFilterResourceMethod = -1;
            ntStatus = STATUS_SUCCESS;
        }

        // If the registry entries had the wrong values, set them to the ones
        // we need here

        if (NT_SUCCESS(ntStatus) && OldStateEnableConnectInterruptIoctl != 1)
        {
            ntStatus = cbm_registry_write_ulong(handleReg, L"EnableConnectInterruptIoctl", 1);
        }

        if (NT_SUCCESS(ntStatus) && OldStateFilterResourceMethod != 2)
        {
            ntStatus = cbm_registry_write_ulong(handleReg, L"FilterResourceMethod", 2);
        }

        // we're done, we do not need the hardware key anymore

        cbm_registry_close_hardwarekey(handleReg, pdo);
    }

    FUNC_LEAVE_NTSTATUS(ntStatus);
}
