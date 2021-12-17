/*
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2001 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Copyright 2001-2005,2007-2010 Spiro Trikaliotis
 *
*/

/*! ************************************************************** 
** \file lib/plugin/xa1541/WINDOWS/iec.c \n
** \author Michael Klein, Spiro Trikaliotis \n
** \n
** \brief Shared library / DLL for accessing the driver, windows specific code
**
****************************************************************/

#include <windows.h>
#include <windowsx.h>

#include <mmsystem.h>

/*! Mark: We are in user-space (for debug.h) */
#define DBG_USERMODE

/*! Mark: We are building the DLL */
#define DBG_DLL

/*! The name of the executable */
#define DBG_PROGNAME "OPENCBM.DLL"

/*! This file is "like" debug.c, that is, define some variables */
#define DBG_IS_DEBUG_C

#include "debug.h"

#include <winioctl.h>
#include "cbmioctl.h"

#include <stdlib.h>

//! mark: We are building the DLL */
#define DLL
#include "i_opencbm.h"

//! mark: We are building the DLL */
#define OPENCBM_PLUGIN
#include "archlib.h"


/*! \internal \brief Start fast scheduling

 The cbm4win driver is very timing sensitive. Because of this, it is
 very important that the scheduling is done as fast as possible.

 Unfortunately, Windows has a varying scheduling granularity. Normally,
 it is in the order of 5 us to 20 us, but it can even grow much larger,
 for example, on an NT4 system or an an SMP or HT machine.

 fastschedule_start() uses a function from mmsystem.h which tells
 Windows to schedule with a much lower granularity. In this case, we ask
 Windows to use a granularity of 1 us.

 Note that this has a negative impact on the overall performance of the
 system, as more scheduling decisions have to be taken by the system.
 Anyway, it has a very positive impact for cbm4win.
 
 Note: Every call to fastschedule_start() has to be balanced with an
 appropriate call to fastschedule_stop() 
*/

static void
fastschedule_start(void)
{
    FUNC_ENTER();

    if (timeBeginPeriod(1) != TIMERR_NOERROR)
    {
        DBG_WARN((DBG_PREFIX "Unable to decrease scheduling period."));
    }

    FUNC_LEAVE();
}

/*! \internal \brief End fast scheduling

 For an explanation of this function, see fastschedule_start().
*/

static void
fastschedule_stop(void)
{
    FUNC_ENTER();

    if (timeEndPeriod(1) != TIMERR_NOERROR)
    {
        DBG_WARN((DBG_PREFIX "Unable to restore scheduling period."));
    }

    FUNC_LEAVE();
}


/*! \brief DLL initialization und unloading

 This function is called whenever the DLL is loaded or unloaded.
 It ensures that the driver is loaded to be able to call its
 functions.

 \param Module
   Handle of the module; this is not used.

 \param Reason
   DLL_PROCESS_ATTACH if the DLL is loaded,
   DLL_PROCESS_DETACH if it is unloaded.

 \param Reserved
   Not used.

 \return 
   Returns TRUE on success, else FALSE.

 If this function returns FALSE, windows reports that loading the DLL
 was not successful. If the DLL is linked statically, the executable
 refuses to load with STATUS_DLL_INIT_FAILED (0xC0000142)
*/

BOOL WINAPI
DllMain(IN HANDLE Module, IN DWORD Reason, IN LPVOID Reserved)
{
    static BOOL bIsOpen = FALSE;
    BOOLEAN Status = TRUE;

    FUNC_ENTER();

#if DBG

    if (Reason == DLL_PROCESS_ATTACH)
    {
        // Read the debugging flags from the registry

        cbm_get_debugging_flags("xa1541");
    }

#endif

    /* make sure the definitions in opencbm.h and cbmioctl.h
     * match each other! 
     * Since we are the only instance which includes both files,
     * we are the only one which can ensure this.
     */

    DBG_ASSERT(IEC_LINE_CLOCK == IEC_CLOCK);
    DBG_ASSERT(IEC_LINE_RESET == IEC_RESET);
    DBG_ASSERT(IEC_LINE_DATA == IEC_DATA);
    DBG_ASSERT(IEC_LINE_ATN == IEC_ATN);

    switch (Reason) 
    {
        case DLL_PROCESS_ATTACH:

            if (IsDriverStartedAutomatically())
            {
                // the driver is started automatically, do not try
                // to start it

                Status = TRUE;
            }
            else
            {
                if (bIsOpen)
                {
                    DBG_ERROR((DBG_PREFIX "No multiple instances are allowed!"));
                    Status = FALSE;
                }
                else
                {
                    Status  = TRUE;
                    bIsOpen = cbm_driver_start();
                }
            }

            WaitForIoCompletionInit();

            /* If the DLL loaded successfully, ask for fast scheduling */
            if (Status)
            {
                fastschedule_start();
            }
            break;

        case DLL_PROCESS_DETACH:

            if (IsDriverStartedAutomatically())
            {
                // the driver is started automatically, do not try
                // to stop it

                Status = TRUE;
            }
            else
            {
                if (!bIsOpen)
                {
                    DBG_ERROR((DBG_PREFIX "Driver is not running!"));
                    Status = FALSE;
                }
                else
                {
                    // it is arguable if the driver should be stopped
                    // whenever the DLL is unloaded.

                    cbm_driver_stop();
                    bIsOpen = FALSE;
                }
            }

            /* If the DLL unloaded successfully, we do not need fast scheduling anymore. */
            if (Status)
            {
                fastschedule_stop();
            }

            WaitForIoCompletionDeinit();
            break;

        default:
            break;

    }

    FUNC_LEAVE_BOOL(Status);
}

/*! \brief Lock the parallel port for the driver

 This function locks the driver onto the parallel port. This way,
 no other program or driver can allocate the parallel port and
 interfere with the communication.

 \param HandleDevice  
   A CBM_FILE which contains the file handle of the driver.

 If cbm_driver_open() did not succeed, it is illegal to 
 call cbm_driver_close().

 \remark
 A call to cbm_lock() is undone with a call to cbm_unlock().

 Note that it is *not* necessary to call this function
 (or cbm_unlock()) when all communication is done with
 the handle to opencbm open (that is, between 
 cbm_driver_open() and cbm_driver_close(). You only
 need this function to pin the driver to the port even
 when cbm_driver_close() is to be executed (for example,
 because the program terminates).
*/

void CBMAPIDECL
opencbm_plugin_lock(CBM_FILE HandleDevice)
{
    FUNC_ENTER();

    cbm_ioctl(HandleDevice, CBMCTRL(PARPORT_LOCK), NULL, 0, NULL, 0);

    FUNC_LEAVE();
}

/*! \brief Unlock the parallel port for the driver

 This function unlocks the driver from the parallel port.
 This way, other programs and drivers can allocate the
 parallel port and do their own communication with
 whatever device they use.

 \param HandleDevice  
   A CBM_FILE which contains the file handle of the driver.

 If cbm_driver_open() did not succeed, it is illegal to 
 call cbm_driver_close().

 \remark
 Look at cbm_lock() for an explanation of this function.
*/

void CBMAPIDECL
opencbm_plugin_unlock(CBM_FILE HandleDevice)
{
    FUNC_ENTER();

    cbm_ioctl(HandleDevice, CBMCTRL(PARPORT_UNLOCK), NULL, 0, NULL, 0);

    FUNC_LEAVE();
}

/*! \brief Write data to the IEC serial bus

 This function sends data after a cbm_listen().

 \param HandleDevice  
   A CBM_FILE which contains the file handle of the driver.

 \param Buffer
   Pointer to a buffer which hold the bytes to write to the bus.

 \param Count
   Number of bytes to be written.

 \return
   >= 0: The actual number of bytes written. 
   <0  indicates an error.

 This function tries to write Count bytes. Anyway, if an error
 occurs, this function can stop prematurely.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL
opencbm_plugin_raw_write(CBM_FILE HandleDevice, const void *Buffer, size_t Count)
{
    DWORD BytesWritten;
    OVERLAPPED overlapped;
    BOOL result;

    FUNC_ENTER();

    WaitForIoCompletionConstruct(&overlapped);

    result = WriteFile(
        HandleDevice,
        Buffer,
        Count,
        &BytesWritten,
        &overlapped
        );

    WaitForIoCompletion(result, HandleDevice, &overlapped, &BytesWritten);

    FUNC_LEAVE_INT(BytesWritten);
}

/*! \brief Read data from the IEC serial bus

 This function retrieves data after a cbm_talk().

 \param HandleDevice 
   A CBM_FILE which contains the file handle of the driver.

 \param Buffer
   Pointer to a buffer which will hold the bytes read.

 \param Count
   Number of bytes to be read at most.

 \return
   >= 0: The actual number of bytes read. 
   <0  indicates an error.

 At most Count bytes are read.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL
opencbm_plugin_raw_read(CBM_FILE HandleDevice, void *Buffer, size_t Count)
{
    DWORD bytesToRead = Count;
    DWORD bytesRead;

    OVERLAPPED overlapped;
    BOOL result;

    FUNC_ENTER();

    WaitForIoCompletionConstruct(&overlapped);

    result = ReadFile(
        HandleDevice,
        Buffer,
        bytesToRead,
        &bytesRead,
        &overlapped
        );

    WaitForIoCompletion(result, HandleDevice, &overlapped, &bytesRead);

    FUNC_LEAVE_INT(bytesRead);
}



/*! \brief Send a LISTEN on the IEC serial bus

 This function sends a LISTEN on the IEC serial bus.
 This prepares a LISTENer, so that it will wait for our
 bytes we will write in the future.

 \param HandleDevice  
   A CBM_FILE which contains the file handle of the driver.

 \param DeviceAddress
   The address of the device on the IEC serial bus. This
   is known as primary address, too.

 \param SecondaryAddress
   The secondary address for the device on the IEC serial bus.

 \return
   0 means success, else failure

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL
opencbm_plugin_listen(CBM_FILE HandleDevice, unsigned char DeviceAddress, unsigned char SecondaryAddress)
{
    CBMT_LISTEN_IN parameter;
    int returnValue;

    FUNC_ENTER();

    parameter.PrimaryAddress = DeviceAddress;
    parameter.SecondaryAddress = SecondaryAddress;

    returnValue = cbm_ioctl(HandleDevice, CBMCTRL(LISTEN), &parameter, sizeof(parameter), NULL, 0)
        ? 0 : 1;

    FUNC_LEAVE_INT(returnValue);
}

/*! \brief Send a TALK on the IEC serial bus

 This function sends a TALK on the IEC serial bus.
 This prepares a TALKer, so that it will prepare to send
 us some bytes in the future.

 \param HandleDevice  
   A CBM_FILE which contains the file handle of the driver.

 \param DeviceAddress
   The address of the device on the IEC serial bus. This
   is known as primary address, too.

 \param SecondaryAddress
   The secondary address for the device on the IEC serial bus.

 \return
   0 means success, else failure

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL
opencbm_plugin_talk(CBM_FILE HandleDevice, unsigned char DeviceAddress, unsigned char SecondaryAddress)
{
    CBMT_TALK_IN parameter;
    int returnValue;

    FUNC_ENTER();

    parameter.PrimaryAddress = DeviceAddress;
    parameter.SecondaryAddress = SecondaryAddress;

    returnValue = cbm_ioctl(HandleDevice, CBMCTRL(TALK), &parameter, sizeof(parameter), NULL, 0)
        ? 0 : 1;

    FUNC_LEAVE_INT(returnValue);
}

/*! \brief Open a file on the IEC serial bus

 This function opens a file on the IEC serial bus.

 \param HandleDevice  
   A CBM_FILE which contains the file handle of the driver.

 \param DeviceAddress
   The address of the device on the IEC serial bus. This
   is known as primary address, too.

 \param SecondaryAddress
   The secondary address for the device on the IEC serial bus.

 \return
   0 means success, else failure

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL
opencbm_plugin_open(CBM_FILE HandleDevice, unsigned char DeviceAddress, unsigned char SecondaryAddress)
{
    CBMT_OPEN_IN parameter;
    int returnValue;

    FUNC_ENTER();

    parameter.PrimaryAddress = DeviceAddress;
    parameter.SecondaryAddress = SecondaryAddress;

    if (cbm_ioctl(HandleDevice, CBMCTRL(OPEN), &parameter, sizeof(parameter), NULL, 0))
    {
        returnValue = 0;
    }
    else
    {
        returnValue = -1;
    }

    FUNC_LEAVE_INT(returnValue);
}

/*! \brief Close a file on the IEC serial bus

 This function closes a file on the IEC serial bus.

 \param HandleDevice  
   A CBM_FILE which contains the file handle of the driver.

 \param DeviceAddress
   The address of the device on the IEC serial bus. This
   is known as primary address, too.

 \param SecondaryAddress
   The secondary address for the device on the IEC serial bus.

 \return
   0 on success, else failure

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL
opencbm_plugin_close(CBM_FILE HandleDevice, unsigned char DeviceAddress, unsigned char SecondaryAddress)
{
    CBMT_CLOSE_IN parameter;
    int returnValue;

    FUNC_ENTER();

    parameter.PrimaryAddress = DeviceAddress;
    parameter.SecondaryAddress = SecondaryAddress;

    returnValue = 
        cbm_ioctl(HandleDevice, CBMCTRL(CLOSE), &parameter, sizeof(parameter), NULL, 0)
        ? 0 : 1;

    FUNC_LEAVE_INT(returnValue);
}

/*! \brief Send an UNLISTEN on the IEC serial bus

 This function sends an UNLISTEN on the IEC serial bus.
 Other than LISTEN and TALK, an UNLISTEN is not directed
 to just one device, but to all devices on that IEC
 serial bus. 

 \param HandleDevice  
   A CBM_FILE which contains the file handle of the driver.

 \return
   0 on success, else failure

 At least on a 1541 floppy drive, an UNLISTEN also undoes
 a previous TALK.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL
opencbm_plugin_unlisten(CBM_FILE HandleDevice)
{
    int returnValue;

    FUNC_ENTER();

    returnValue = cbm_ioctl(HandleDevice, CBMCTRL(UNLISTEN), NULL, 0, NULL, 0) ? 0 : 1;

    FUNC_LEAVE_INT(returnValue);
}

/*! \brief Send an UNTALK on the IEC serial bus

 This function sends an UNTALK on the IEC serial bus.
 Other than LISTEN and TALK, an UNTALK is not directed
 to just one device, but to all devices on that IEC
 serial bus. 

 \param HandleDevice  
   A CBM_FILE which contains the file handle of the driver.

 \return
   0 on success, else failure

 At least on a 1541 floppy drive, an UNTALK also undoes
 a previous LISTEN.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL
opencbm_plugin_untalk(CBM_FILE HandleDevice)
{
    int returnValue;

    FUNC_ENTER();

    returnValue = cbm_ioctl(HandleDevice, CBMCTRL(UNTALK), NULL, 0, NULL, 0) ? 0 : 1;

    FUNC_LEAVE_INT(returnValue);
}


/*! \brief Get EOI flag after bus read

 This function gets the EOI ("End of Information") flag 
 after reading the IEC serial bus.

 \param HandleDevice  
   A CBM_FILE which contains the file handle of the driver.

 \return
   != 0 if EOI was signalled, else 0.

 If a previous read returned less than the specified number
 of bytes, there are two possible reasons: Either an error
 occurred on the IEC serial bus, or an EOI was signalled.
 To find out the cause, check with this function.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL
opencbm_plugin_get_eoi(CBM_FILE HandleDevice)
{
    CBMT_GET_EOI_OUT result;

    FUNC_ENTER();

    cbm_ioctl(HandleDevice, CBMCTRL(GET_EOI), NULL, 0, &result, sizeof(result));

    FUNC_LEAVE_INT(result.Decision);
}

/*! \brief Reset the EOI flag

 This function resets the EOI ("End of Information") flag
 which might be still set after reading the IEC serial bus.

 \param HandleDevice  
   A CBM_FILE which contains the file handle of the driver.

 \return
   0 on success, != 0 means an error has occured.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL
opencbm_plugin_clear_eoi(CBM_FILE HandleDevice)
{
    int returnValue;

    FUNC_ENTER();

    returnValue = cbm_ioctl(HandleDevice, CBMCTRL(CLEAR_EOI), NULL, 0, NULL, 0);

    FUNC_LEAVE_INT(returnValue);
}

/*! \brief RESET all devices

 This function performs a hardware RESET of all devices on
 the IEC serial bus.

 \param HandleDevice  
   A CBM_FILE which contains the file handle of the driver.

 \return
   0 on success, else failure

 Don't overuse this function! Normally, an initial RESET
 should be enough.

 Control is returned after a delay which ensures that all
 devices are ready again.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL
opencbm_plugin_reset(CBM_FILE HandleDevice)
{
    USHORT returnValue;

    FUNC_ENTER();

    //
    // try to cancel any pending io operations.
    //
    WaitForIoCompletionCancelAll();

    returnValue = cbm_ioctl(HandleDevice, CBMCTRL(RESET), NULL, 0, NULL, 0) ? 0 : 1;

    FUNC_LEAVE_INT(returnValue);
}


/*-------------------------------------------------------------------*/
/*--------- LOW-LEVEL PORT ACCESS -----------------------------------*/

/*! \brief Read a byte from a XP1541/XP1571 cable

 This function reads a single byte from the parallel portion of 
 an XP1541/1571 cable.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \return
   the byte which was received on the parallel port

 This function reads the current state of the port. No handshaking
 is performed at all.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 \bug
   This function can't signal an error, thus, be careful!
*/

unsigned char CBMAPIDECL
opencbm_plugin_pp_read(CBM_FILE HandleDevice)
{
    CBMT_PP_READ_OUT result;

    FUNC_ENTER();

    cbm_ioctl(HandleDevice, CBMCTRL(PP_READ), NULL, 0, &result, sizeof(result));

    FUNC_LEAVE_UCHAR(result.Byte);
}

/*! \brief Write a byte to a XP1541/XP1571 cable

 This function writes a single byte to the parallel portion of 
 a XP1541/1571 cable.

 \param HandleDevice

   A CBM_FILE which contains the file handle of the driver.

 \param Byte

   the byte to be output on the parallel port

 This function just writes on the port. No handshaking
 is performed at all.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 \bug
   This function can't signal an error, thus, be careful!
*/

void CBMAPIDECL
opencbm_plugin_pp_write(CBM_FILE HandleDevice, unsigned char Byte)
{
    CBMT_PP_WRITE_IN parameter;

    FUNC_ENTER();

    parameter.Byte = Byte;

    cbm_ioctl(HandleDevice, CBMCTRL(PP_WRITE), &parameter, sizeof(parameter), NULL, 0);

    FUNC_LEAVE();
}

/*! \brief Read status of all bus lines.

 This function reads the state of all lines on the IEC serial bus.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \return
   The state of the lines. The result is an OR between
   the bit flags IEC_DATA, IEC_CLOCK, IEC_ATN, and IEC_RESET.

 This function just reads the port. No handshaking
 is performed at all.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 \bug
   This function can't signal an error, thus, be careful!
*/

int CBMAPIDECL
opencbm_plugin_iec_poll(CBM_FILE HandleDevice)
{
    CBMT_IEC_POLL_OUT result;

    FUNC_ENTER();

    cbm_ioctl(HandleDevice, CBMCTRL(IEC_POLL), NULL, 0, &result, sizeof(result));

    FUNC_LEAVE_INT(result.Line);
}


/*! \brief Activate a line on the IEC serial bus

 This function activates (sets to 0V) a line on the IEC serial bus.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Line
   The line to be activated. This must be exactly one of
   IEC_DATA, IEC_CLOCK, IEC_ATN, or IEC_RESET.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 \bug
   This function can't signal an error, thus, be careful!
*/

void CBMAPIDECL
opencbm_plugin_iec_set(CBM_FILE HandleDevice, int Line)
{
    CBMT_IEC_SET_IN parameter;

    FUNC_ENTER();
 
    parameter.Line = (UCHAR) Line;

    cbm_ioctl(HandleDevice, CBMCTRL(IEC_SET), &parameter, sizeof(parameter), NULL, 0);

    FUNC_LEAVE();
}

/*! \brief Deactivate a line on the IEC serial bus

 This function deactivates (sets to 5V) a line on the IEC serial bus.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Line
   The line to be deactivated. This must be exactly one of
   IEC_DATA, IEC_CLOCK, IEC_ATN, or IEC_RESET.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 \bug
   This function can't signal an error, thus, be careful!
*/

void CBMAPIDECL
opencbm_plugin_iec_release(CBM_FILE HandleDevice, int Line)
{
    CBMT_IEC_RELEASE_IN parameter;

    FUNC_ENTER();

    parameter.Line = (UCHAR) Line;

    cbm_ioctl(HandleDevice, CBMCTRL(IEC_RELEASE), &parameter, sizeof(parameter), NULL, 0);

    FUNC_LEAVE();
}

/*! \brief Activate and deactive a line on the IEC serial bus

 This function activates (sets to 0V, L) and deactivates 
 (set to 5V, H) lines on the IEC serial bus.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Set
   The mask of which lines should be set. This has to be a bitwise OR
   between the constants IEC_DATA, IEC_CLOCK, IEC_ATN, and IEC_RESET

 \param Release
   The mask of which lines should be released. This has to be a bitwise
   OR between the constants IEC_DATA, IEC_CLOCK, IEC_ATN, and IEC_RESET

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 \bug
   This function can't signal an error, thus, be careful!

 \remark
   If a bit is specified in the Set as well as in the Release mask, the
   effect is undefined.
*/

void CBMAPIDECL
opencbm_plugin_iec_setrelease(CBM_FILE HandleDevice, int Set, int Release)
{
    CBMT_IEC_SETRELEASE_IN parameter;

    FUNC_ENTER();
 
    parameter.State = (UCHAR) Set;
    parameter.Line = (UCHAR) Release;

    cbm_ioctl(HandleDevice, CBMCTRL(IEC_SETRELEASE), &parameter, sizeof(parameter), NULL, 0);

    FUNC_LEAVE();
}

/*! \brief Wait for a line to have a specific state

 This function waits for a line to enter a specific state
 on the IEC serial bus.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Line
   The line to be deactivated. This must be exactly one of
   IEC_DATA, IEC_CLOCK, IEC_ATN, and IEC_RESET.

 \param State
   If zero, then wait for this line to be deactivated. \n
   If not zero, then wait for this line to be activated.

 \return
   The state of the IEC bus on return (like cbm_iec_poll).

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 \bug
   This function can't signal an error, thus, be careful!
*/

int CBMAPIDECL
opencbm_plugin_iec_wait(CBM_FILE HandleDevice, int Line, int State)
{
    CBMT_IEC_WAIT_IN parameter;
    CBMT_IEC_WAIT_OUT result;

    FUNC_ENTER();

    parameter.Line = (UCHAR) Line;
    parameter.State = (UCHAR) State;

    cbm_ioctl(HandleDevice, CBMCTRL(IEC_WAIT), 
        &parameter, sizeof(parameter), 
        &result, sizeof(result));

    FUNC_LEAVE_INT(result.Line);
}

#if DBG

/*! \brief Output contents of the debugging buffer

 This function outputs the contents of the kernel-mode
 debugging buffer to the screen.

 This function is for use of the installation routines only!
*/

int CBMAPIDECL
cbm_get_debugging_buffer(CBM_FILE HandleDevice, char *buffer, size_t len)
{
    FUNC_ENTER();

    cbm_ioctl(HandleDevice, CBMCTRL(I_READDBG), NULL, 0, buffer, len);

    FUNC_LEAVE_INT(0);
}

#endif // #if DBG

/*! \brief Read a byte from the parallel port input register

 This function reads a byte from the parallel port input register.
 (STATUS_PORT). It is a helper function for debugging the cable
 (i.e., for the XCDETECT tool) only!

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \return
   If the routine succeeds, it returns a non-negative value
   which corresponds to the data in the parallel port input
   register (status port).

   If the routine fails, the return value is -1.

 \remark
   Do not use this function in anything but a debugging aid tool
   like XCDETECT!

   This functions masks some bits off. The bits that are not masked
   off are defined in PARALLEL_STATUS_PORT_MASK_VALUES.
*/
int CBMAPIDECL
opencbm_plugin_iec_dbg_read(CBM_FILE HandleDevice)
{
    CBMT_IEC_DBG_READ result;
    int returnValue = -1;

    FUNC_ENTER();

    if ( cbm_ioctl(HandleDevice, CBMCTRL(IEC_DBG_READ), NULL, 0, &result, sizeof(result)) ) {
        returnValue = result.Value;
    }

    FUNC_LEAVE_INT(returnValue);
}

/*! \brief Write a byte to the parallel port output register

 This function writes a byte to the parallel port output register.
 (CONTROL_PORT). It is a helper function for debugging the cable
 (i.e., for the XCDETECT tool) only!

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Value
   The value to set the control port to

 \return 
   If the routine succeeds, it returns 0.
   
   If the routine fails, it returns -1.

 \remark
   Do not use this function in anything but a debugging aid tool
   like XCDETECT!

   After this function has been called, it is NOT safe to use the
   parallel port again unless you close the driver (cbm_driver_close())
   and open it again (cbm_driver_open())!

   This functions masks some bits off. That is, the bits not in the
   mask are not changed at all. The bits that are not masked
   off are defined in PARALLEL_CONTROL_PORT_MASK_VALUES.
*/
int CBMAPIDECL
opencbm_plugin_iec_dbg_write(CBM_FILE HandleDevice, unsigned char Value)
{
    CBMT_IEC_DBG_WRITE parameter;
    int returnValue = -1;

    FUNC_ENTER();

    parameter.Value = Value;

    if ( cbm_ioctl(HandleDevice, CBMCTRL(IEC_DBG_WRITE), &parameter, sizeof(parameter), NULL, 0) ) {
        returnValue = 0;
    }

    FUNC_LEAVE_INT(returnValue);
}
