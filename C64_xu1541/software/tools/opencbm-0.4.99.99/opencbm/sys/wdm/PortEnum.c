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
** \file sys/wdm/PortEnum.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Functions for communicating with the parallel port driver
**
****************************************************************/

#include <initguid.h>
#include <wdm.h>

#define PENUMERATE_DEFINED //!< Make sure the PENUMERATE is correctly defined
typedef struct ENUMERATE_WDM *PENUMERATE; //!< pointer to a ENUMERATE_WDM or ENUMERATE_NT4 struct
#include "cbm_driver.h"

/*! This struct is used as internal storage for
 * the ParPortEnumerateOpen()/ParPortEnumerate()/ParPortEnumerateClose()
 * functions
 */
typedef
struct ENUMERATE_WDM
{
    /*! Pointer to a list of symbolic links which contains all
     *  driver names which implement the parallel port interface
     */
    PWSTR SymbolicLinkList;

    /*! Pointer to the current entry in the SymbolicLinkList */
    PWSTR CurrentSymbolicLink;
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
        // Mark: We did not yet enumerate the first entry

        enumStruct->CurrentSymbolicLink = NULL;

        // Get every device which implementes our interface

        DBG_IRQL( == PASSIVE_LEVEL);
        ntStatus = IoGetDeviceInterfaces(&GUID_PARALLEL_DEVICE,
            NULL, // no pdo given
            0,
            &enumStruct->SymbolicLinkList);
    }
    else 
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    // didn't we succeed? Then return with the correct value

    if (!NT_SUCCESS(ntStatus))  
    {
        // We had a problem obtaining the devices which implement the
        // parallel port interface, thus, fail this function

        if (enumStruct)
        {
            // Free the enumStruct

            DBG_IRQL( < DISPATCH_LEVEL);
            ExFreePool(enumStruct);

            enumStruct = NULL;
        }
    }

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
    FUNC_ENTER();

    DBG_ASSERT(EnumStruct != NULL);
    DBG_ASSERT(EnumStruct->SymbolicLinkList != NULL);

    // Advance EnumStruct->CurrentSymbolicLink to the next entry

    if (EnumStruct->CurrentSymbolicLink == NULL)
    {
        // we just started enumerating, get the first entry

        EnumStruct->CurrentSymbolicLink = EnumStruct->SymbolicLinkList;
    }
    else
    {
        // advance to the next entry

        EnumStruct->CurrentSymbolicLink += wcslen(EnumStruct->CurrentSymbolicLink)+1;
    }

    // Give the next driver name to the caller

    *DriverName = EnumStruct->CurrentSymbolicLink;

    FUNC_LEAVE_NTSTATUS((**DriverName) ? STATUS_SUCCESS : STATUS_NO_MORE_ENTRIES);
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
    DBG_ASSERT(EnumStruct->SymbolicLinkList != NULL);

    // Free both allocated buffers

    DBG_IRQL( < DISPATCH_LEVEL);
    ExFreePool(EnumStruct->SymbolicLinkList);
    ExFreePool(EnumStruct);

    DBGDO(EnumStruct = NULL;)

    FUNC_LEAVE();
}

/*! \brief Stub for a function

 This function is not available on NT4, and it is not
 needed there, but it is needed for WDM. Because of this, we define 
 it here for cbm4wdm.sys, so that the driver works in either case.
*/
NTSTATUS 
CbmOpenDeviceRegistryKey(IN PDEVICE_OBJECT a, IN ULONG b, IN ACCESS_MASK c, OUT PHANDLE d)
{
    FUNC_ENTER();

    FUNC_LEAVE_NTSTATUS(IoOpenDeviceRegistryKey(a, b, c, d));
}
