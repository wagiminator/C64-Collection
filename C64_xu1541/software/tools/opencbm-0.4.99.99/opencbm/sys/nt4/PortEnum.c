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
** \file sys/nt4/PortEnum.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Functions for communicating with the parallel port driver
**
****************************************************************/

#include <initguid.h>
#include <ntddk.h>

#define PENUMERATE_DEFINED //!< Make sure the PENUMERATE is correctly defined
typedef struct ENUMERATE_NT4 *PENUMERATE; //!< pointer to a ENUMERATE_WDM or ENUMERATE_NT4 struct
#include "cbm_driver.h"

#undef ExFreePool

/*! This struct is used as internal storage for
 * the ParPortEnumerateOpen()/ParPortEnumerate()/ParPortEnumerateClose()
 * functions
 */
typedef
struct ENUMERATE_NT4
{
    //! Prefix of the device names to be returned
    UNICODE_STRING DriverPrefix;

    //! Memory for the number in the device names
    UNICODE_STRING DriverNumber;

    //! The complete driver name which will be returned
    UNICODE_STRING CompleteDriverName;

    //! The actual number which the functions just returned
    ULONG Count;

    //! The maximum number which the functions might return
    ULONG MaxCount;

} ENUMERATE, *PENUMERATE;


/*! \brief Start enumeration of the parallel port drivers

 This function starts the enumeration process of the parallel port drivers

 \param EnumStruct
   Pointer to a pointer to hold an ENUMERATE structure. 
   This is internal storage for these functions, and it is allocated
   by this function.

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. Otherwise, it
   returns one of the error status values.

 For enumerating, a caller has to do the following:
 - Allocate memory for an ENUMERATE structure \n
 - Call ParPortEnumerateOpen() with the allocated memory \n
 - Call ParPortEnumerate() in a loop to get each next parallel port,
       until an error code shows that there is no more parallel port. \n
 - Close the enumeration process by ParPortEnumerateClose()
*/
NTSTATUS
ParPortEnumerateOpen(PENUMERATE *EnumStruct)
{
    PENUMERATE enumStruct;
    NTSTATUS ntStatus;

    FUNC_ENTER();

    // Allocate memory for the enumStruct

    DBG_IRQL( < DISPATCH_LEVEL);
    enumStruct = ExAllocatePoolWithTag(PagedPool, 
        sizeof(ENUMERATE), MTAG_SENUMERATE);

    if (enumStruct)
    {
        // Zero the complete enumStruct

        RtlZeroMemory(enumStruct, sizeof(*enumStruct));

        enumStruct->Count = 0;

        // Get the maximum number of parallel port devices

        DBG_IRQL( == PASSIVE_LEVEL);
        enumStruct->MaxCount = IoGetConfigurationInformation()->ParallelCount;

        // If the maximum number is bigger than 999, we have a severe problem...
        // Who would put more than 999 parallel ports into one machine?
        // Anyway, make sure we have no problems with this.

        if (enumStruct->MaxCount > 999)
        {
            DBG_WARN((DBG_PREFIX "IoGetConfigurationInformation()->ParallelCount "
                "returned %u, truncating to 999", enumStruct->MaxCount));

            enumStruct->MaxCount = 999;
        }

        // prepare the strings for later usage

        DBG_IRQL( <= DISPATCH_LEVEL);
        RtlInitUnicodeString(&enumStruct->DriverPrefix, L"\\Device\\" DD_PARALLEL_PORT_BASE_NAME_U);

        // Allocate memory for storing the number
        // Assume the number is < 999, that is, not more than 3 digits big
        // If it is bigger, make the numbers 3 and 4 greater
        // The length has to be set here because the allocation for 
        // enumStruct->CompleteDriverName below depends upon the length!

        enumStruct->DriverNumber.Length = 3*sizeof(wchar_t);
        enumStruct->DriverNumber.MaximumLength = 4*sizeof(wchar_t);
        enumStruct->DriverNumber.Buffer = ExAllocatePoolWithTag(PagedPool,
            enumStruct->DriverNumber.MaximumLength, MTAG_SENUMERATE);

        // allocate memory for the complete name

        enumStruct->CompleteDriverName.Length = 0;
        enumStruct->CompleteDriverName.MaximumLength = 
            enumStruct->DriverPrefix.Length + enumStruct->DriverNumber.Length;

        enumStruct->CompleteDriverName.Buffer = ExAllocatePoolWithTag(PagedPool,
            enumStruct->CompleteDriverName.MaximumLength, MTAG_SENUMERATE);

        // Test if both above allocations were successfull:

        if (enumStruct->DriverNumber.Buffer != NULL 
            && enumStruct->CompleteDriverName.Buffer != NULL)
        {
            ntStatus = STATUS_SUCCESS;
        }
        else
        {
            // We have not had luck with allocating both strings,
            // thus, return an appropriate error value

            ntStatus = STATUS_INSUFFICIENT_RESOURCES;

            // Free all resources that have been successfully allocated

            ParPortEnumerateClose(enumStruct);

            // return that there was no enumStruct allocated

            enumStruct = NULL;
        }
    }
    else
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    // return the enumStruct to the caller

    *EnumStruct = enumStruct;

    FUNC_LEAVE_NTSTATUS(ntStatus);
}

/*! \brief Get next enumerated parallel port driver

 This function starts the enumeration process of the parallel port drivers

 \param EnumStruct
   Pointer to a ENUMERATE structure. This is internal storage
   for these functions.

 \param DriverName;
   Pointer to a storage area which will contain the next
   driver implementing this interface.

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. Otherwise, it
   returns one of the error status values.

 For enumerating, a caller has to do the following:
 - Allocate memory for an ENUMERATE structure \n
 - Call ParPortEnumerateOpen() with the allocated memory \n
 - Call ParPortEnumerate() in a loop to get each next parallel port,
       until an error code shows that there is no more parallel port. \n
 - Close the enumeration process by ParPortEnumerateClose()
*/
NTSTATUS
ParPortEnumerate(PENUMERATE EnumStruct, PCWSTR *DriverName)
{
    NTSTATUS ntStatus;
    PCWSTR retDriver;

    FUNC_ENTER();

    DBG_ASSERT(EnumStruct != NULL);
    DBG_ASSERT(EnumStruct->Count < 1000);

    retDriver = NULL;

    // Assume: There are no more entries available

    ntStatus = STATUS_NO_MORE_ENTRIES;

    if (EnumStruct->Count < EnumStruct->MaxCount)
    {
        // Counvert EnumStruct->Count to a UNICODE_STRING

        DBG_IRQL( == PASSIVE_LEVEL);
        ntStatus = RtlIntegerToUnicodeString(EnumStruct->Count, 10, 
            &EnumStruct->DriverNumber);

        if (NT_SUCCESS(ntStatus)) 
        {
            // Copy the parts to form the full name:

            DBG_IRQL( < DISPATCH_LEVEL);
            RtlCopyUnicodeString(&EnumStruct->CompleteDriverName, &EnumStruct->DriverPrefix);
            RtlAppendUnicodeStringToString(&EnumStruct->CompleteDriverName, &EnumStruct->DriverNumber);

            retDriver = EnumStruct->CompleteDriverName.Buffer;

            // advance to the next driver

            ++EnumStruct->Count;
        }
    }
    else
    {
        // we are after the last entry: return empty string

        EnumStruct->CompleteDriverName.Buffer[0] = 0;
        retDriver = EnumStruct->CompleteDriverName.Buffer;
    }

    *DriverName = retDriver;

    FUNC_LEAVE_NTSTATUS(ntStatus);
}

/*! \brief Stop enumeration of the parallel port drivers

 This function stops the enumeration process of the parallel port drivers

 \param EnumStruct
   Pointer to a ENUMERATE structure, which contains the last
   enumerated driver

 For enumerating, a caller has to do the following:
 - Allocate memory for an ENUMERATE structure \n
 - Call ParPortEnumerateOpen() with the allocated memory \n
 - Call ParPortEnumerate() in a loop to get each next parallel port,
       until an error code shows that there is no more parallel port. \n
 - Close the enumeration process by ParPortEnumerateClose()
*/
VOID
ParPortEnumerateClose(PENUMERATE EnumStruct)
{
    FUNC_ENTER();

    DBG_ASSERT(EnumStruct != NULL);

    DBG_IRQL( == PASSIVE_LEVEL);

    // Free the DriverNumber buffer, if that was allocated

    if (EnumStruct->DriverNumber.Buffer != NULL)
    {
        DBG_IRQL( < DISPATCH_LEVEL);
        ExFreePool(EnumStruct->DriverNumber.Buffer);
    }

    // Free the CompleteDriverName buffer, if that was allocated

    if (EnumStruct->CompleteDriverName.Buffer != NULL)
    {
        DBG_IRQL( < DISPATCH_LEVEL);
        ExFreePool(EnumStruct->CompleteDriverName.Buffer);
    }

    // Free the EnumStruct

    DBG_IRQL( < DISPATCH_LEVEL);
    ExFreePool(EnumStruct);

    FUNC_LEAVE();
}

/*! \brief Stub for a function

 This function is not available on NT4, and it is not
 needed. Because of this, we define it here for cbm4nt.sys,
 so that the driver successfully loads
*/
NTSTATUS 
CbmOpenDeviceRegistryKey(IN PDEVICE_OBJECT a, IN ULONG b, IN ACCESS_MASK c, OUT PHANDLE d)
{
    FUNC_ENTER();

    DBG_ASSERT(1==0);

    FUNC_LEAVE_NTSTATUS_CONST(STATUS_SUCCESS);
}
