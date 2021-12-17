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
** \file sys/libcommon/util-reg.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Some utility functions for accessing the registry
**
****************************************************************/

#include <wdm.h>
#include "cbm_driver.h"

/*! \brief Open a registry path for reading

 This function opens a registry key ("path"). This way, its entries 
 can be read afterwards.

 \param HandleKey
   Pointer to a HANDLE which will contain the handle to the
   registry key ("path") on exit

 \param Path
   Pointer to a UNICODE_STRING which points to the registry key
   to be opened.

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. 
   Otherwise, it return one of the error status values.

 A call to this function must be balanced with a call to
 cbm_registry_close().
*/
NTSTATUS
cbm_registry_open_for_read(OUT PHANDLE HandleKey, IN PUNICODE_STRING Path)
{
    OBJECT_ATTRIBUTES objectAttributes;

    NTSTATUS ntStatus;

    FUNC_ENTER();

    FUNC_PARAM((DBG_PREFIX "'%wZ'", Path));

    // Initialize the object attributes for the registry key

    DBG_IRQL( == PASSIVE_LEVEL); //+< according to NT4 DDK; later ones do not have this restriction!
    InitializeObjectAttributes(&objectAttributes,
        Path, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    // Now, open the registry key

    DBG_IRQL( == PASSIVE_LEVEL);
    ntStatus = ZwOpenKey(HandleKey, KEY_READ, &objectAttributes);

    FUNC_LEAVE_NTSTATUS(ntStatus);
}

/*! \brief Open the hardware key for another driver

 This function opens a the "hardware" registry key for the
 parallel port driver.

 \param HandleKey
   Pointer to a HANDLE which will contain the handle to the
   registry key ("path") on exit

 \param Pdo
   Pointer to the DEVICE_OBJECT for which we found the hardware key.

 \param Pdx
   The PDX of the driver which is calling this function

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. 
   Otherwise, it return one of the error status values.

 A call to this function must be balanced with a call to
 cbm_registry_close_hardwarekey().

 Thanks to Doron Holan [MS] for pointing out how to do it
 (MsgId:<O6weQL8qEHA.3428@TK2MSFTNGP11.phx.gbl>
 on microsoft.public.development.device.drivers, 
 http://groups.google.com/groups?selm=O6weQL8qEHA.3428%40TK2MSFTNGP11.phx.gbl
*/
NTSTATUS
cbm_registry_open_hardwarekey(OUT PHANDLE HandleKey, OUT PDEVICE_OBJECT *Pdo,
                              IN PDEVICE_EXTENSION Pdx)
{
    IO_STATUS_BLOCK ioStatusBlock;
    NTSTATUS ntStatus;
    KEVENT event;
    PIRP irp;

    FUNC_ENTER();

    // Initialize the event which might be needed for waiting for completion

    // no IRQL restrictions apply
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    // first of all, build an IRP with IRP_MJ_PNP/IRP_MN_QUERY_DEVICE_RELATIONS

    DBG_IRQL( == PASSIVE_LEVEL);
    irp = IoBuildSynchronousFsdRequest(
        IRP_MJ_PNP,
        Pdx->ParallelPortFdo,
        NULL,
        0,
        NULL,
        &event,
        &ioStatusBlock);

    if (!irp)
    {
        // Acquiring the IRP failed, report this back to the caller

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        PIO_STACK_LOCATION irpSp;

        // set up some values for the called driver

        // We need to write the first IRP stack location, thus, get
        // it. On a new IRP, this means we want to write the "next"
        // stack location.

        DBG_IRQL( <= DISPATCH_LEVEL);
        irpSp = IoGetNextIrpStackLocation(irp);

        if (irpSp)
        {
            // Find the PDO of the parallel port driver

            irpSp->MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS;
            irpSp->Parameters.QueryDeviceRelations.Type = TargetDeviceRelation;
            irpSp->FileObject = Pdx->ParallelPortFileObject;

            // call the driver to get the handle for the hardware registry key

            DBG_IRQL( <= DISPATCH_LEVEL);
            ntStatus = IoCallDriver(Pdx->ParallelPortFdo, irp);
        }
        else
        {
            // Well... We could allocate an IRP, but we could not
            // get the appropriate stack location? Can this ever happen?
            // I doubt it, but to be sure, mark it as an error

            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }

        // If the called driver pended this IRP, wait for the completion

        if (ntStatus == STATUS_PENDING)
        {
            DBG_IRQL( < DISPATCH_LEVEL);
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        }

        if (NT_SUCCESS(ntStatus))
        {
            PDEVICE_RELATIONS deviceRelations;
            PDEVICE_OBJECT pdo;

            // use deviceRelations for better readability

            deviceRelations = (PDEVICE_RELATIONS) ioStatusBlock.Information;
            DBG_ASSERT(deviceRelations);

            if (deviceRelations)
            {
                // take the pdo out of the return value

                pdo = deviceRelations->Objects[0];

                // get a registry handle for the hardware key of the parallel port

                DBG_IRQL( == PASSIVE_LEVEL);
                ntStatus = CbmOpenDeviceRegistryKey(
                    pdo,
                    PLUGPLAY_REGKEY_DEVICE,
                    KEY_ALL_ACCESS,
                    HandleKey);

                // give the pdo back to the caller, so that it can be dereferenced when it
                // is not longer needed

                *Pdo = pdo;

                // free the memory which is no longer needed

                DBG_IRQL( < DISPATCH_LEVEL);
                ExFreePool(deviceRelations);
            }
        }
    }


    FUNC_LEAVE_NTSTATUS(ntStatus);
}

/*! \brief Close a registry key

 This function closes a registry key.

 \param HandleKey
   The HANDLE of the registry path.

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. 
   Otherwise, it return one of the error status values.

 If the registry key was obtained with a call to
 cbm_registry_open_hardwarekey(), you must *not* use
 this function, but use cbm_registry_close_hardwarekey()
 instead.
*/
NTSTATUS
cbm_registry_close(IN HANDLE HandleKey)
{
    NTSTATUS ntStatus;

    FUNC_ENTER();

    // Close the registry key

    DBG_IRQL( == PASSIVE_LEVEL);
    ntStatus = ZwClose(HandleKey);

    FUNC_LEAVE_NTSTATUS(ntStatus);
}

/*! \brief Close a hardware registry key

 This function closes a registry key pointing
 to a hardware registry key.

 \param HandleKey
   The HANDLE of the registry path.

 \param Pdo
   Pointer to the DEVICE_OBJECT for which we found the hardware key.

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. 
   Otherwise, it return one of the error status values.
*/
NTSTATUS
cbm_registry_close_hardwarekey(IN HANDLE HandleKey, IN PDEVICE_OBJECT Pdo)
{
    NTSTATUS ntStatus;

    FUNC_ENTER();

    // First of all, close the registry key

    ntStatus = cbm_registry_close(HandleKey);

    // Dereference the pdo, as we do not need it anymore.

    DBG_IRQL( <= DISPATCH_LEVEL);
    ObDereferenceObject(Pdo);

    FUNC_LEAVE_NTSTATUS(ntStatus);
}

/*! \brief Read a ULONG value out of a registry key

 This function reads a ULONG value out of the registry.

 \param HandleKey
   The HANDLE of the registry path.

 \param KeyName
   A null-terminated wide char string which contains the name of the
   value to be read.

 \param Value
   Pointer to a memory location which will hold the return value.

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. 
   Otherwise, it return one of the error status values.

 If the value cannot be read, the return value in *Value will not be changed.
*/
NTSTATUS
cbm_registry_read_ulong(IN HANDLE HandleKey, IN PCWSTR KeyName, OUT PULONG Value)
{
    PKEY_VALUE_PARTIAL_INFORMATION p;
    UNICODE_STRING keyNameUnicode;
    NTSTATUS ntStatus;
    ULONG retValueSize;
    CHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION)+sizeof(ULONG)];

    FUNC_ENTER();

    FUNC_PARAM((DBG_PREFIX "'%ws'", KeyName));

    p = (PKEY_VALUE_PARTIAL_INFORMATION) buffer;

    RtlInitUnicodeString(&keyNameUnicode, KeyName);

    ntStatus = ZwQueryValueKey(HandleKey, &keyNameUnicode, 
        KeyValuePartialInformation, 
        buffer, sizeof(buffer), &retValueSize);

    if (NT_SUCCESS(ntStatus))
    {
        if (p->Type == REG_DWORD && p->DataLength == sizeof(ULONG))
        {
            ULONG retValue = *((PULONG) &p->Data);

            FUNC_PARAM((DBG_PREFIX "Return-Value: %08x", retValue));

            if (Value)
            {
                *Value = retValue;
            }
        }
    }

    FUNC_LEAVE_NTSTATUS(ntStatus);
}

/*! \brief Write a ULONG value out of a registry key

 This function writes a ULONG value to the registry.

 \param HandleKey
   The HANDLE of the registry path.

 \param KeyName
   A null-terminated wide char string which contains the name of the
   value to be read.

 \param Value
   The value to be written to that memory entry.

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. 
   Otherwise, it return one of the error status values.
*/
NTSTATUS
cbm_registry_write_ulong(IN HANDLE HandleKey, IN PCWSTR KeyName, IN ULONG Value)
{
    PKEY_VALUE_PARTIAL_INFORMATION p;
    UNICODE_STRING keyNameUnicode;
    NTSTATUS ntStatus;
    ULONG retValueSize;
    CHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION)+sizeof(ULONG)];

    FUNC_ENTER();

    FUNC_PARAM((DBG_PREFIX "'%ws'", KeyName));

    p = (PKEY_VALUE_PARTIAL_INFORMATION) buffer;

    RtlInitUnicodeString(&keyNameUnicode, KeyName);

    ntStatus = ZwSetValueKey(HandleKey, &keyNameUnicode,
        0, REG_DWORD, &Value, sizeof(Value));

    FUNC_LEAVE_NTSTATUS(ntStatus);
}
