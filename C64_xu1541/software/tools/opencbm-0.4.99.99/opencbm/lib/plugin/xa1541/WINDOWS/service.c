/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2004, 2008, 2012 Spiro Trikaliotis
 *
 */

/*! ************************************************************** 
** \file lib/plugin/xa1541/WINDOWS/service.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Functions for accessing the service control manager for the OPENCBM driver
**
****************************************************************/


#include <windows.h>
#include <stdio.h>
#include "cbmioctl.h"

// #include "instcbm.h"

#include "i_opencbm.h"

#include "libmisc.h"

/*! Mark: We are in user-space (for debug.h) */
#define DBG_USERMODE

/*! The name of the executable */
#define DBG_PROGNAME "OPENCBM-XA1541.DLL"

#include "debug.h"

/*! \internal \brief Set a DWORD value in the registry

 This function sets a DWORD value in the registry. It is a simple
 wrapper for convenience.

 \param RegKey
   A handle to an already opened registry key.

 \param SubKey
   Pointer to a null-terminiated string which holds the name
   of the value to be created or changed.

 \param Value
   The value the registry setting is to be set to.
*/

static VOID
RegSetDWORD(HKEY RegKey, char *SubKey, DWORD Value)
{
    DWORD rc;

    FUNC_ENTER();

    DBG_PRINT((DBG_PREFIX "Setting %s to %u", SubKey, Value));

    rc = RegSetValueEx(RegKey, SubKey, 0, REG_DWORD, (LPBYTE)&Value, 4);

    DBG_ASSERT(rc == ERROR_SUCCESS);

    FUNC_LEAVE();
}

/*! \internal \brief Set an EXPANDSZ value in the registry

 This function sets an EXPANDSZ value in the registry. 
 It is a simple wrapper for convenience.

 \param RegKey
   A handle to an already opened registry key.

 \param SubKey
   Pointer to a null-terminiated string which holds the name
   of the value to be created or changed.

 \param Value
   The value the registry setting is to be set to.
*/

static VOID
RegSetEXPANDSZ(HKEY RegKey, char *SubKey, IN LPCTSTR Value)
{
    DWORD rc;

    FUNC_ENTER();

    rc = RegSetValueEx(RegKey, SubKey, 0, REG_EXPAND_SZ, Value, strlen(Value)+1);

    DBG_ASSERT(rc == ERROR_SUCCESS);

    FUNC_LEAVE();
}


/*! \internal \brief Create the registry keys for the event service

 This function sets some registry keys, so our driver can issue
 events which are expanded afterwards.

 \param ServiceExe
   The path to the executable which contains the logging texts

 \return
   TRUE if an error occurred, else FALSE.
*/

static BOOL
CreateLogRegistryKeys(IN LPCTSTR ServiceExe)
{
    DWORD dwDisposition;
    HKEY RegKey;
    BOOL error;

    FUNC_ENTER();

    error = TRUE;

    do {

        // Open a registry key to HKLM\<%REGKEY_EVENTLOG%>

        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,                         
                           REGKEY_EVENTLOG,
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_SET_VALUE,
                           NULL,
                           &RegKey,
                           &dwDisposition
                          )
           )
        {
            RegKey = NULL;
            break;
        }  

        // Store the path to the file which contains the event service entries

        RegSetEXPANDSZ(RegKey, "EventMessageFile", ServiceExe);

        // The written "FACILITY" is 0x07 (FACILITY_PARALLEL_ERROR_CODE)

        RegSetDWORD(RegKey, "TypesSupported", 0x07);

        error = FALSE;

    } while (0);

    // We're done, close the registry handle.

    if (RegKey) {
        RegCloseKey(RegKey);
    }

    FUNC_LEAVE_BOOL(error);
}

/*! \internal \brief Create a registry keys for default LPT

 This function sets a registry key which contains the
 default LPT.

 \param DefaultLpt
   The number of the default LPT

 \param IecCableType
   The IEC cable type

 \param PermanentlyLock
   Zero if driver should not permanently lock the parallel port,
   not zero otherwise.

 \param DebugFlagsDriverPresent
   Specifies if the following DebugFlagsDriver field should be used or not

 \param DebugFlagsDriver
   The DebugFlags to be set for the driver

 \param DebugFlagsDllPresent
   Specifies if the following DebugFlagsDll field should be used or not

 \param DebugFlagsDll
   The DebugFlags to be set for the DLL

 \return
   TRUE on success, else FALSE.
*/

static BOOL
CreateDefaultRegistryKeys(IN ULONG DefaultLpt,
                          IN ULONG IecCableType,
                          IN ULONG PermanentlyLock,
                          IN BOOL DebugFlagsDriverPresent, IN ULONG DebugFlagsDriver,
                          IN BOOL DebugFlagsDllPresent, IN ULONG DebugFlagsDll)
{
    BOOLEAN success;
    HKEY RegKey;

    FUNC_ENTER();

    success = FALSE;

    // Open a registry key to HKLM\<%CBM_REGKEY_SERVICE%>

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     CBM_REGKEY_SERVICE,
                     0,
                     KEY_ALL_ACCESS,
                     &RegKey) 
        != ERROR_SUCCESS)

    {
        DWORD error = GetLastError();

        DBG_WARN((DBG_PREFIX "COULD NOT OPEN HKLM\\" CBM_REGKEY_SERVICE " (0x%x) '%s'",
            error, cbmlibmisc_format_error_message(error)));
        printf("WARNING: COULD NOT OPEN HKLM\\" CBM_REGKEY_SERVICE " (0x%x) '%s'\n",
            error, cbmlibmisc_format_error_message(error));
    }
    else
    {
        // Set the value for DefaultLpt

        if (DefaultLpt != -1)
        {
            RegSetDWORD(RegKey, CBM_REGKEY_SERVICE_DEFAULTLPT, DefaultLpt);
        }

        if (IecCableType != IEC_CABLETYPE_UNSPEC)
        {
            RegSetDWORD(RegKey, CBM_REGKEY_SERVICE_IECCABLE, IecCableType);
        }

        if (PermanentlyLock != -1)
        {
            RegSetDWORD(RegKey, CBM_REGKEY_SERVICE_PERMLOCK, PermanentlyLock);
        }

#if DBG

        // Set the value for the DebugFlags

        if (DebugFlagsDriverPresent)
        {
            RegSetDWORD(RegKey, CBM_REGKEY_SERVICE_DEBUGFLAGS, DebugFlagsDriver);
        }

        if (DebugFlagsDllPresent)
        {
            RegSetDWORD(RegKey, CBM_REGKEY_SERVICE_DLL_DEBUGFLAGS, DebugFlagsDll);
        }

#endif // #if DBG

        // We're done, close the registry handle.

        RegCloseKey(RegKey);

        success = TRUE;
    }

    FUNC_LEAVE_BOOL(success);
}


/*! \brief Install the driver

 This function installs the opencbm driver on the
 machine.

 \param DriverName
   The name under which the driver should be installed.

 \param ServiceExe
   The path to the executable which contains the logging texts

 \param AutomaticStart
   If set to TRUE, then the driver start type should be set
   to "AUTOMATIC", that is, the driver is started on every boot.
   If FALSE, it is put to "MANUAL".

 \return
   TRUE if an error occurred, else FALSE.
*/

BOOL
CbmInstall(IN LPCTSTR DriverName, IN LPCTSTR ServiceExe, IN BOOL AutomaticStart)
{
    SC_HANDLE scManager;
    SC_HANDLE scService;
    DWORD lasterror;
    BOOL error;

    FUNC_ENTER();

    error = FALSE;

    scManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (scManager)
    {
        // Create the service

        scService = CreateService(scManager, DriverName, DriverName, 
           SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, 
           AutomaticStart ? SERVICE_AUTO_START : SERVICE_DEMAND_START,
           SERVICE_ERROR_NORMAL, ServiceExe, 
           "Extended base", NULL, "+Parallel arbitrator\0Parport\0",
           NULL, NULL);

        if (scService == NULL)
        {
            lasterror = GetLastError();

            if (lasterror == ERROR_SERVICE_EXISTS)
            {
                //  This is not an error, so process it differently from the others.

                DBG_WARN((DBG_PREFIX "CreateService (0x%02x) '%s'",
                    lasterror, cbmlibmisc_format_error_message(lasterror)));
                printf("WARNING: opencbm is already installed!\n");
            }
            else
            {
                DBG_ERROR((DBG_PREFIX "CreateService (0x%02x) '%s'",
                    lasterror, cbmlibmisc_format_error_message(lasterror)));
                printf("ERROR: CreateService (0x%02x) '%s'\n",
                    lasterror, cbmlibmisc_format_error_message(lasterror));

                error = TRUE;
            }
        }
        else
        {
            DBG_SUCCESS((DBG_PREFIX "CreateService"));
            CloseServiceHandle(scService);
        }

        // Create the registry setting for the default LPT port

        CreateDefaultRegistryKeys(-1, -2, -1, FALSE, 0, FALSE, 0);

        CloseServiceHandle(scManager);

        // Create the registry settings for being able to output to the
        // event service

        error = CreateLogRegistryKeys(ServiceExe);

        // If the driver is to be started automatically, start it now

        if (AutomaticStart)
        {
            cbm_driver_start();
        }
    }
    else
    {
        error = TRUE;
    }

    FUNC_LEAVE_BOOL(error);
}


/*! \brief Update the parameter of the driver

 This function updates the parameters of the driver.

 \param DefaultLpt
   The default LPT to be set

 \param IecCableType
   The IEC cable type

 \param PermanentlyLock
   Zero if driver should not permanently lock the parallel port,
   not zero otherwise.

 \param DebugFlagsDriverPresent
   Specifies if the following DebugFlagsDriver field should be used or not

 \param DebugFlagsDriver
   The DebugFlags to be set for the driver

 \param DebugFlagsDllPresent
   Specifies if the following DebugFlagsDll field should be used or not

 \param DebugFlagsDll
   The DebugFlags to be set for the DLL

 \return
   TRUE on success, else FALSE.
*/

BOOL
CbmUpdateParameter(IN ULONG DefaultLpt,
                   IN ULONG IecCableType,
                   IN ULONG PermanentlyLock,
                   IN BOOL DebugFlagsDriverPresent, IN ULONG DebugFlagsDriver,
                   IN BOOL DebugFlagsDllPresent, IN ULONG DebugFlagsDll)
{
    BOOL ret;

    FUNC_ENTER();

    ret = CreateDefaultRegistryKeys(DefaultLpt, IecCableType, PermanentlyLock,
                DebugFlagsDriverPresent, DebugFlagsDriver,
                DebugFlagsDllPresent, DebugFlagsDll);

//    CbmInstallUpdate();

    FUNC_LEAVE_BOOL(ret);
}


/*! \brief Remove the driver

 This function removes the opencbm driver from the
 machine.

 \param DriverName
   The name under which the driver is installed.

 \return
   TRUE on success, else FALSE.
*/

BOOL
CbmRemove(IN LPCTSTR DriverName)
{
    SC_HANDLE scManager;
    SC_HANDLE scService;
    BOOL ret;

    FUNC_ENTER();

    // Make sure the driver is stopped before being unloaded

    cbm_driver_stop();

    scManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (scManager)
    {
        scService = OpenService(scManager, DriverName, SERVICE_ALL_ACCESS);

        if (scService == NULL)
        {
            DWORD error = GetLastError();

            DBG_ERROR((DBG_PREFIX "OpenService (0x%02x) '%s'", error, cbmlibmisc_format_error_message(error)));
            printf("ERROR: OpenService (0x%02x) '%s'\n", error, cbmlibmisc_format_error_message(error));
            FUNC_LEAVE_BOOL(FALSE);
        }

        ret = DeleteService(scService);

        if (ret)
        {
            DBG_SUCCESS((DBG_PREFIX "DeleteService"));
            printf("DeleteService SUCCESS\n");
        }
        else
        {
            DWORD error = GetLastError();

            DBG_ERROR((DBG_PREFIX "DeleteService (0x%02x) '%s'", error, cbmlibmisc_format_error_message(error)));
            printf("ERROR: DeleteService (0x%02x) '%s'\n", error, cbmlibmisc_format_error_message(error));
        }

        CloseServiceHandle(scService);


        // Remove the registry settings which were created for the event service

        RegDeleteKey(HKEY_LOCAL_MACHINE, REGKEY_EVENTLOG);

        CloseServiceHandle(scManager);
    }
    else
    {
        ret = FALSE;
    }

    FUNC_LEAVE_BOOL(ret);
}


/*! \brief Check for the presence of the driver

 This function checks if the driver is present.

 \param DriverName
   The name under which the driver is installed.

 \return
   TRUE if the driver is present, FALSE is not, or if
   there was an error.
*/

BOOL
CbmCheckPresence(IN LPCTSTR DriverName)
{
    SC_HANDLE scManager;
    SC_HANDLE scService;

    FUNC_ENTER();

    scService = NULL;

    scManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (scManager)
    {
        scService = OpenService(scManager, DriverName, SERVICE_ALL_ACCESS);

        if (scService != NULL)
        {
            CloseServiceHandle(scService);
        }

        CloseServiceHandle(scManager);
    }

    FUNC_LEAVE_BOOL(scService ? TRUE : FALSE);
}
