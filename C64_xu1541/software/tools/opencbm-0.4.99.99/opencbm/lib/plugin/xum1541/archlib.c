/*
 *  xum1541 plugin interface, derived from the xu1541 file of the same name
 *
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *  Copyright 2009-2010 Nate Lawson
 *  Copyright 1999-2001 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Copyright 2001-2005, 2007, 2010-2011 Spiro Trikaliotis
 *  Copyright 2010 Wolfgang Moser
 *
*/

/*! ************************************************************** 
** \file lib/plugin/xum1541/archlib.c \n
** \author Michael Klein, Spiro Trikaliotis \n
** \n
** \brief Shared library / DLL for accessing the driver, windows specific code
**
****************************************************************/

#ifdef WIN32
#include <windows.h>
#include <windowsx.h>

/*! Mark: We are in user-space (for debug.h) */
#define DBG_USERMODE

/*! Mark: We are building the DLL */
// #define DBG_DLL

/*! The name of the executable */
#define DBG_PROGNAME "OPENCBM-XUM1541.DLL"

/*! This file is "like" debug.c, that is, define some variables */
// #define DBG_IS_DEBUG_C

#include "debug.h"
#endif

#include <stdlib.h>

//! mark: We are building the DLL */
#define OPENCBM_PLUGIN
#include "archlib_ex.h"

#include "xum1541.h"


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
   PortNumber is not allowed to exceed 255. 
*/

const char * CBMAPIDECL
opencbm_plugin_get_driver_name(const char * const Port)
{
    int portNumber = 0;
    
    if(Port != NULL) {
        portNumber = strtoul(Port, NULL, 10);
    }

    return xum1541_device_path(portNumber);
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

 PortNumber is not allowed to exceed 10. 

 cbm_driver_open() should be balanced with cbm_driver_close().
*/

int CBMAPIDECL
opencbm_plugin_driver_open(CBM_FILE *HandleDevice, const char * const Port)
{
    int portNumber = 0;
    
    if(Port != NULL) {
        portNumber = strtoul(Port, NULL, 10);
    }

    return xum1541_init((usb_dev_handle **)HandleDevice, portNumber);
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
    xum1541_close((usb_dev_handle *)HandleDevice);
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
    return xum1541_write((usb_dev_handle *)HandleDevice, XUM1541_CBM, Buffer, Count);
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
    return xum1541_read((usb_dev_handle *)HandleDevice, XUM1541_CBM, Buffer, Count);
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
    unsigned char proto, dataBuf[2];

    proto = XUM1541_CBM | XUM_WRITE_ATN;
    dataBuf[0] = 0x20 | DeviceAddress;
    dataBuf[1] = 0x60 | SecondaryAddress;
    return !xum1541_write((usb_dev_handle *)HandleDevice, proto, dataBuf, sizeof(dataBuf));
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
    unsigned char proto, dataBuf[2];

    proto = XUM1541_CBM | XUM_WRITE_ATN | XUM_WRITE_TALK;
    dataBuf[0] = 0x40 | DeviceAddress;
    dataBuf[1] = 0x60 | SecondaryAddress;
    return !xum1541_write((usb_dev_handle *)HandleDevice, proto, dataBuf, sizeof(dataBuf));
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
    unsigned char proto, dataBuf[2];

    proto = XUM1541_CBM | XUM_WRITE_ATN;
    dataBuf[0] = 0x20 | DeviceAddress;
    dataBuf[1] = 0xf0 | SecondaryAddress;
    return !xum1541_write((usb_dev_handle *)HandleDevice, proto, dataBuf, sizeof(dataBuf));
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
    unsigned char proto, dataBuf[2];

    proto = XUM1541_CBM | XUM_WRITE_ATN;
    dataBuf[0] = 0x20 | DeviceAddress;
    dataBuf[1] = 0xe0 | SecondaryAddress;
    return !xum1541_write((usb_dev_handle *)HandleDevice, proto, dataBuf, sizeof(dataBuf));
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
    unsigned char proto, dataBuf[1];

    proto = XUM1541_CBM | XUM_WRITE_ATN;
    dataBuf[0] = 0x3f;
    return !xum1541_write((usb_dev_handle *)HandleDevice, proto, dataBuf, sizeof(dataBuf));
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
    unsigned char proto, dataBuf[1];

    proto = XUM1541_CBM | XUM_WRITE_ATN;
    dataBuf[0] = 0x5f;
    return !xum1541_write((usb_dev_handle *)HandleDevice, proto, dataBuf, sizeof(dataBuf));
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
    return xum1541_ioctl((usb_dev_handle *)HandleDevice, XUM1541_GET_EOI, 0, 0);
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
    return xum1541_ioctl((usb_dev_handle *)HandleDevice, XUM1541_CLEAR_EOI, 0, 0);
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
    return xum1541_control_msg((usb_dev_handle *)HandleDevice, XUM1541_RESET);
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
    return (unsigned char) xum1541_ioctl((usb_dev_handle *)HandleDevice, XUM1541_PP_READ, 0, 0);
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
    xum1541_ioctl((usb_dev_handle *)HandleDevice, XUM1541_PP_WRITE, Byte, 0);
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
    return xum1541_ioctl((usb_dev_handle *)HandleDevice, XUM1541_IEC_POLL, 0, 0);
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
    xum1541_ioctl((usb_dev_handle *)HandleDevice, XUM1541_IEC_SETRELEASE, Line, 0);
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
    xum1541_ioctl((usb_dev_handle *)HandleDevice, XUM1541_IEC_SETRELEASE, 0, Line);
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
    xum1541_ioctl((usb_dev_handle *)HandleDevice, XUM1541_IEC_SETRELEASE, Set, Release);
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
    return xum1541_ioctl((usb_dev_handle *)HandleDevice, XUM1541_IEC_WAIT, Line, State);
}

/*! \brief Sends a command to the xum1541 device

 This function sends a control message respectively a command to the xum1541 device.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param cmd
   The command to run at the xum1541 device.

 \return
   Returns the value the USB device sent back.
*/

int CBMAPIDECL
xum1541_plugin_control_msg(CBM_FILE HandleDevice, unsigned int cmd)
{
    return xum1541_control_msg((usb_dev_handle *)HandleDevice, cmd);
}
