/*
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2001 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Copyright 2001-2004, 2007-2009, 2011 Spiro Trikaliotis
 *
 *  Parts are Copyright
 *      Jouko Valta <jopi(at)stekt(dot)oulu(dot)fi>
 *      Andreas Boose <boose(at)linux(dot)rz(dot)fh-hannover(dot)de>
*/

/*! ************************************************************** 
** \file lib/plugin/xa1541/WINDOWS/i_opencbm.c \n
** \author Spiro Trikaliotis \n
** \authors Based on code from
**    Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
** \n
** \brief Helper functions for the DLL for accessing the driver,
**        and the install functions
**
****************************************************************/

#include <windows.h>
#include <windowsx.h>

/*! Mark: We are in user-space (for debug.h) */
#define DBG_USERMODE

/*! The name of the executable */
#ifndef DBG_PROGNAME
    #define DBG_PROGNAME "OPENCBM-XA1541.DLL"
#endif // #ifndef DBG_PROGNAME

#include "debug.h"

#include <winioctl.h>
#include "cbmioctl.h"

#include <stdlib.h>
#include <stddef.h>

#include "i_opencbm.h"

#include "version.h"

#define OPENCBM_PLUGIN /*!< \brief mark: we are exporting plugin functions */

#include "archlib.h"


/*-------------------------------------------------------------------*/
/*--------- REGISTRY FUNCTIONS --------------------------------------*/

/*! \internal \brief Get the number of the parallel port to open

 This function checks the registry for the number of the parallel port 
 to be opened as default.

 \return 
   Returns the number of the parallel port to be opened as default,
   starting with 0.

 If the registry entry does not exist, this function returns 0, which
 is also the default after installing the driver.
*/

static int
cbm_get_default_port(VOID)
{
    DWORD ret;
    HKEY regKey;

    FUNC_ENTER();

    DBG_PPORT((DBG_PREFIX "cbm_get_default_port()"));

    // Open a registry key to HKLM\<%REGKEY_SERVICE%>

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     CBM_REGKEY_SERVICE,
                     0,
                     KEY_QUERY_VALUE,
                     &regKey)
       )
    {
        DBG_WARN((DBG_PREFIX "RegOpenKeyEx() failed!"));
        FUNC_LEAVE_BOOL(FALSE);
    }

    // now, get the number of the port to use

    if (RegGetDWORD(regKey, CBM_REGKEY_SERVICE_DEFAULTLPT, &ret) != ERROR_SUCCESS)
    {
        DBG_WARN((DBG_PREFIX "No " CBM_REGKEY_SERVICE "\\" CBM_REGKEY_SERVICE_DEFAULTLPT 
            " value, setting 0."));
        ret = 0;
    }

    // We're done, close the registry handle.

    RegCloseKey(regKey);

    DBG_PPORT((DBG_PREFIX "RETURN: cbm_get_default_port() == %u", ret));

    FUNC_LEAVE_INT(ret);
}

/*-------------------------------------------------------------------*/
/*--------- ASYNCHRONOUS IO FUNCTIONS -------------------------------*/

/*! A special "cancel event" which is signalled whenever we want to
 * prematurely cancel an I/O request  */

static HANDLE CancelEvent = NULL;
static HANDLE CancelCallbackEvent = NULL;
static ULONG  InCancellableState  = 0;

/*! \brief Initialize WaitForIoCompletion()

 This function initializes everything needed for the 
 WaitForIoCompletion...() functions. It has to be called
 exactly once for each program start.
*/

VOID
WaitForIoCompletionInit(VOID)
{
    FUNC_ENTER();

    //
    // Create the events for prematurely cancelling I/O request
    //

    CancelEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    DBG_ASSERT(CancelEvent != NULL);

    CancelCallbackEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    DBG_ASSERT(CancelCallbackEvent != NULL);

    FUNC_LEAVE();
}

/*! \brief Uninitialize WaitForIoCompletion()

 This function uninitializes everything needed for the 
 WaitForIoCompletion...() functions. It has to be called
 exactly once for each program stop.

 \remark
   Make sure there are no outstanding I/O requests when this
   function is called!
*/

VOID
WaitForIoCompletionDeinit(VOID)
{
    FUNC_ENTER();

    //
    // Delete the event which is used for prematurely 
    // cancelling I/O request
    //
    if (CancelEvent != NULL)
        CloseHandle(CancelEvent);

    FUNC_LEAVE();
}

/*! \brief Cancel any running WaitForIoCompletion()

 This function cancels the running WaitForIoCompletion()
 function.
*/

VOID
WaitForIoCompletionCancelAll(VOID)
{
    FUNC_ENTER();

    if (InterlockedExchange(&InCancellableState, 0) != 0)
    {
        //
        // signal the event which is used for prematurely 
        // cancelling I/O request
        //

        SetEvent(CancelEvent);

        //
        // Wait to be signalled that the current I/O request
        // has been cancelled.
        //

        WaitForSingleObject(CancelCallbackEvent, INFINITE);
    }

    FUNC_LEAVE();
}

/*! \brief Boilerplate code for asynchronous I/O requests

 This function initializes 

 \param Overlapped
   Pointer to an OVERLAPPED structure that will be initialized.

 \remark
   This function completely initializes an OVERLAPPED structure
   to be used with ReadFile(), WriteFile, DeviceIoControl()
   later, and waited for with WaitForIoCompletion().
*/

VOID
WaitForIoCompletionConstruct(LPOVERLAPPED Overlapped)
{
    FUNC_ENTER();

    memset(Overlapped, 0, sizeof(*Overlapped));
    Overlapped->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    DBG_ASSERT(Overlapped->hEvent != NULL);

    FUNC_LEAVE();
}

/*! \brief Wait for the completion of an I/O operation

 This function waits until an I/O operation is completed,
 or cancelled.

 \param Result
   The result of the previous I/O operation 
   (ReadFile(), WriteFile(), DeviceIoControl())

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Overlapped
   Pointer to an OVERLAPPED structure that was specified when the
   overlapped operation was started.

 \param BytesTransferred
   Pointer to a DWORD which will contain the number of bytes
   transferred in this asynchronous I/O operation.

 \return 
   FALSE if a failure occurred, TRUE if success.

 \remark
   A cancelled request is considered as a failure, thus, FALSE
   is returned in this case.
*/

BOOL
WaitForIoCompletion(BOOL Result, CBM_FILE HandleDevice, LPOVERLAPPED Overlapped, DWORD *BytesTransferred)
{
    BOOL result = Result;
    HANDLE handleList[2] = { CancelEvent, Overlapped->hEvent };

    FUNC_ENTER();

    DBG_ASSERT(Overlapped != NULL);
    DBG_ASSERT(BytesTransferred != NULL);

    if (!Result)
    {
        // deal with the error code 
        switch (GetLastError()) 
        { 
            case ERROR_IO_PENDING:
            {
                int tmp;

                //
                // Make sure WaitForIoCompletionCancelAll() knows it has to signal us
                //

                tmp = InterlockedExchange(&InCancellableState, 1);
                DBG_ASSERT(tmp == 0);

                //
                // wait for the operation to finish
                //

                if (WaitForMultipleObjects(2, handleList, FALSE, INFINITE) == WAIT_OBJECT_0)
                {
                    CancelIo(HandleDevice);

                    // we are told to cancel this event
                    *BytesTransferred = 0;
                    result = FALSE;
                    SetEvent(CancelCallbackEvent);
                }
                else
                {
                    //
                    // WaitForIoCompletionCancelAll() does not need to alert us anymore
                    //

                    if (InterlockedExchange(&InCancellableState, 0) == 0)
                    {
                        //
                        // In case we were signalled, make sure 
                        // WaitForIoCompletionCancelAll() does not hang
                        //

                        SetEvent(CancelCallbackEvent);
                    }

                    // check on the results of the asynchronous read 
                    result = GetOverlappedResult(HandleDevice, Overlapped, 
                        BytesTransferred, FALSE) ; 
                }
                break;
            }
         }
    }

    CloseHandle(Overlapped->hEvent);

    FUNC_LEAVE_BOOL(result ? TRUE : FALSE);
}

/*-------------------------------------------------------------------*/
/*--------- OPENCBM ARCH FUNCTIONS ----------------------------------*/

/*! \brief Get the name of the driver for a specific parallel port

 Get the name of the driver for a specific parallel port.

 \param Port
   The port specification for the driver to open. If not set (== NULL),
   the "default" driver is used. The exact meaning depends upon the plugin.

 \return 
   Returns a pointer to a null-terminated string containing the
   driver name, or NULL if an error occurred.

 \bug
   Port is not allowed to exceed 10. 
*/

const char * CBMAPIDECL
opencbm_plugin_get_driver_name(const char * const Port)
{
    //! \todo do not hard-code the driver name
    static char driverName[] = "\\\\.\\opencbm0";
    char *ret;

    int portNumber = 0;

    FUNC_ENTER();

    ret = NULL;

    if (Port != NULL)
    {
        portNumber = strtoul(Port, NULL, 10);
    }

    /*! \bug 
     * the logic does not allow more than 10 entries, 
     * thus, fail this call if we want to use a port > 10!  */

    if (portNumber <= 10)
    {
        if (portNumber == 0)
        {
            // PortNumber 0 has special meaning: Find out the default value

            portNumber = cbm_get_default_port();
        }

        driverName[strlen(driverName)-1] = (portNumber ? portNumber-1 : 0) + '0';
        ret = driverName;
    }

    FUNC_LEAVE_STRING(ret);
}

/*! \brief Opens the driver

 This function Opens the driver.

 \param HandleDevice  
   Pointer to a CBM_FILE which will contain the file handle of the driver.

 \param Port
   The port specification for the driver to open. If not set (== NULL),
   the "default" driver is used. The exact meaning depends upon the plugin.

 \return 
   ==0: This function completed successfully
   !=0: otherwise

 Port is not allowed to exceed 10. 

 cbm_driver_open() should be balanced with cbm_driver_close().
*/

int CBMAPIDECL
opencbm_plugin_driver_open(CBM_FILE *HandleDevice, const char * const Port)
{
    const char *driverName;

    FUNC_ENTER();

    // Get the name of the driver to be opened

    driverName = opencbm_plugin_get_driver_name(Port);

    if (driverName == NULL)
    {
        // there was a problem, thus, fail this call!

        *HandleDevice = INVALID_HANDLE_VALUE;
    }
    else 
    {
        // Open the device

        *HandleDevice = CreateFile(driverName,
            GENERIC_READ | GENERIC_WRITE, // we need read and write access
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED,
            NULL);
    }

    DBG_ASSERT(*HandleDevice != INVALID_HANDLE_VALUE);

    FUNC_LEAVE_INT(*HandleDevice == INVALID_HANDLE_VALUE);
}

/*! \brief Closes the driver

 Closes the driver, which has be opened with cbm_driver_open() before.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 cbm_driver_close() should be called to balance a previous call to
 cbm_driver_open(). 
 
 If cbm_driver_open() did not succeed, it is illegal to 
 call cbm_driver_close().
*/

void CBMAPIDECL
opencbm_plugin_driver_close(CBM_FILE HandleDevice)
{
    FUNC_ENTER();

    DBG_ASSERT(HandleDevice != 0);

    CloseHandle(HandleDevice);

    FUNC_LEAVE();
}

/*! \brief Perform an ioctl on the driver

 This function performs an ioctl on the driver. 
 It is used internally only.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param ControlCode
   The ControlCode of the IOCTL to be performed.

 \param TextControlCode
   A string representation of the IOCTL to be performed. This is
   used for debugging purposes, only, and not available in
   free builds.

 \param InBuffer
   Pointer to a buffer which holds the input parameters for the
   IOCTL. Can be NULL if no input buffer is needed.

 \param InBufferSize
   Size of the buffer pointed to by InBuffer. If InBuffer is NULL,
   this has to be zero,

 \param OutBuffer
   Pointer to a buffer which holds the output parameters of the
   IOCTL. Can be NULL if no output buffer is needed.

 \param OutBufferSize
   Size of the buffer pointed to by OutBuffer. If OutBuffer is NULL,
   this has to be zero,

 \return
   TRUE: IOCTL succeeded, else
   FALSE  an error occurred processing the IOCTL

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

BOOL 
cbm_ioctl(IN CBM_FILE HandleDevice, IN DWORD ControlCode, 
#if DBG
          IN char *TextControlCode, 
#endif // #if DBG
          IN PVOID InBuffer, IN ULONG InBufferSize, OUT PVOID OutBuffer, IN ULONG OutBufferSize)
{
    OVERLAPPED overlapped;
    DWORD dwBytesReturned;

    BOOL returnValue;

    FUNC_ENTER();

    WaitForIoCompletionConstruct(&overlapped);

    // Perform the IOCTL

    returnValue = DeviceIoControl(HandleDevice, ControlCode, InBuffer, InBufferSize,
        OutBuffer, OutBufferSize, &dwBytesReturned, &overlapped);

    returnValue = WaitForIoCompletion(returnValue, HandleDevice, &overlapped, &dwBytesReturned);

    // If an error occurred, output it (in the DEBUG version of this file

    if (!returnValue)
    {
        DBG_ERROR((DBG_PREFIX "%s: Error code = %u", TextControlCode, GetLastError()));
    }
    else
    {
        // Check if the number of bytes returned equals the wanted number

        if (dwBytesReturned != OutBufferSize) 
        {
            DBG_WARN((DBG_PREFIX "%s: OutBufferSize = %u, but dwBytesReturned = %u",
                TextControlCode, OutBufferSize, dwBytesReturned));
        }
    }

    FUNC_LEAVE_BOOL(returnValue);
}

/*-------------------------------------------------------------------*/
/*--------- DEVICE DRIVER HANDLING FUNCTIONS  -----------------------*/


/*! \brief Start a device driver

 This function start a device driver. It is the programmatically
 equivalent for "net start <driver>"

 \return
   Returns TRUE on success, else FALSE.

 This function is for use of the installation routines only!
*/

BOOL
cbm_driver_start(VOID)
{
    SC_HANDLE schManager;
    SC_HANDLE schService;
    DWORD err;
    BOOL ret;

    FUNC_ENTER();

    ret = TRUE;

    schManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (schManager == NULL)
    {
       DBG_ERROR((DBG_PREFIX "Could not open service control manager!"));
       FUNC_LEAVE_BOOL(FALSE);
    }

    schService = OpenService(schManager, OPENCBM_DRIVERNAME, SERVICE_ALL_ACCESS);

    if (schService == NULL)
    {
        DBG_ERROR((DBG_PREFIX "OpenService (0x%02x)", GetLastError()));
        FUNC_LEAVE_BOOL(FALSE);
    }

    ret = StartService(schService, 0, NULL);

    if (ret)
    {
        DBG_SUCCESS((DBG_PREFIX "StartService"));
    }
    else
    {
        err = GetLastError();

        if (err == ERROR_SERVICE_ALREADY_RUNNING)
        {
            /* If the service is already running, don't treat it
             * as an error, but as a warning, and report success!
             */

            DBG_WARN((DBG_PREFIX "StartService, (ERROR_SERVICE_ALREADY_RUNNING)"));

            ret = TRUE;
        }
        else
        {
            DBG_ERROR((DBG_PREFIX "StartService (0x%02x)", err));
        }
    }

    CloseServiceHandle(schService);

    // Close the SCM: We don't need it anymore

    CloseServiceHandle(schManager);

    FUNC_LEAVE_BOOL(ret);
}


/*! \brief Stop a device driver

 This function stops a device driver. It is the programmatically
 equivalent for "net stop <driver>"

 \return
   Returns TRUE on success, else FALSE.

 This function is for use of the installation routines only!
*/

BOOL
cbm_driver_stop(VOID)
{
    SERVICE_STATUS  serviceStatus;
    SC_HANDLE schManager;
    SC_HANDLE schService;
    BOOL ret;

    FUNC_ENTER();

    schManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    schService = OpenService(schManager, OPENCBM_DRIVERNAME, SERVICE_ALL_ACCESS);

    if (schManager == NULL)
    {
       DBG_ERROR((DBG_PREFIX "Could not open service control manager!"));
       FUNC_LEAVE_BOOL(FALSE);
    }

    if (schService == NULL)
    {
        DBG_ERROR((DBG_PREFIX "OpenService (0x%02x)", GetLastError()));
        FUNC_LEAVE_BOOL(FALSE);
    }

    ret = ControlService(schService, SERVICE_CONTROL_STOP, &serviceStatus);

    if (ret)
    {
        DBG_SUCCESS((DBG_PREFIX "ControlService"));
    }
    else
    {
        DBG_ERROR((DBG_PREFIX "ControlService (0x%02x)", GetLastError()));
    }

    CloseServiceHandle(schService);

    // Close the SCM: We don't need it anymore

    CloseServiceHandle(schManager);

    FUNC_LEAVE_BOOL(ret);
}

/*! \brief Complete driver installation, "direct version"

 This function performs anything that is needed to successfully
 complete the driver installation.

 \param Buffer
   Pointer to a buffer which will return the install information

 \param BufferLen
   The length of the buffer Buffer points to (in bytes).

 \return
   FALSE on success, TRUE on error

 This function is for use of the installation routines only!

 This version is for usage in the DLL or the install package.
*/

BOOL
cbm_driver_install(OUT PULONG Buffer, IN ULONG BufferLen)
{
    PCBMT_I_INSTALL_OUT outBuffer;
    CBM_FILE HandleDevice;

    FUNC_ENTER();

    DBG_ASSERT(Buffer != NULL);
    DBG_ASSERT(BufferLen >= sizeof(ULONG));

    outBuffer = (PCBMT_I_INSTALL_OUT) Buffer;

    DBG_ASSERT(outBuffer != NULL);

    if (opencbm_plugin_driver_open(&HandleDevice, 0) == 0)
    {
        outBuffer->ErrorFlags = CBM_I_DRIVER_INSTALL_0_IOCTL_FAILED;
        cbm_ioctl(HandleDevice, CBMCTRL(I_INSTALL), NULL, 0, Buffer, BufferLen);
        opencbm_plugin_driver_close(HandleDevice);
    }
    else
    {
        outBuffer->ErrorFlags = CBM_I_DRIVER_INSTALL_0_FAILED;
        DBG_ERROR((DBG_PREFIX "Driver could not be openend"));
    }

    // if there is room in the given buffer, set the dll version

    if (BufferLen >= sizeof(outBuffer->DllVersion) + offsetof(CBMT_I_INSTALL_OUT, DllVersion))
    {
        outBuffer->DllVersion =
            CBMT_I_INSTALL_OUT_MAKE_VERSION(OPENCBM_VERSION_MAJOR, OPENCBM_VERSION_MINOR,
                                            OPENCBM_VERSION_SUBMINOR, OPENCBM_VERSION_DEVEL);
    }

    // if there is even room for the dll version extension, set the dll version extension

    if (BufferLen >= sizeof(outBuffer->DllVersionEx) + offsetof(CBMT_I_INSTALL_OUT, DllVersionEx))
    {
        outBuffer->DllVersionEx =
            CBMT_I_INSTALL_OUT_MAKE_VERSION_EX(OPENCBM_VERSION_PATCHLEVEL);
    }

    FUNC_LEAVE_INT(
        (  (outBuffer->ErrorFlags != CBM_I_DRIVER_INSTALL_0_IOCTL_FAILED)
        && (outBuffer->ErrorFlags != CBM_I_DRIVER_INSTALL_0_FAILED)
        ) ? FALSE : TRUE);
}

/*! \brief Is the driver started automatically?

 This function finds out if the driver is started automatically
 or manually.

 \return 
   Returns TRUE if driver is started automatically, FALSE if not.
*/

BOOL
IsDriverStartedAutomatically(VOID)
{
    DWORD ret;
    BOOL automaticStart;
    HKEY regKey;

    FUNC_ENTER();

    automaticStart = FALSE;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     CBM_REGKEY_SERVICE,
                     0,
                     KEY_QUERY_VALUE,
                     &regKey)
       )
    {
        DBG_WARN((DBG_PREFIX "RegOpenKeyEx() failed!"));
        FUNC_LEAVE_BOOL(FALSE);
    }

    // now, get the number of the port to use

    if (RegGetDWORD(regKey, "Start", &ret) != ERROR_SUCCESS)
    {
        DBG_ERROR((DBG_PREFIX "No " CBM_REGKEY_SERVICE "\\Start value!"));
    }
    else
    {
        if (ret == SERVICE_AUTO_START)
        {
            automaticStart = TRUE;
        }
    }

    // We're done, close the registry handle.

    RegCloseKey(regKey);

    FUNC_LEAVE_BOOL(automaticStart);
}
