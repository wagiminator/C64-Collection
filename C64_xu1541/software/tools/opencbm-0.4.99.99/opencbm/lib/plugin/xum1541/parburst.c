/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 *  Copyright 2009-2010 Nate Lawson
 */

/*! ************************************************************** 
** \file lib/plugin/xum1541/WINDOWS/parburst.c \n
** \author Nate Lawson \n
** \n
** \brief Shared library / DLL for accessing the mnib driver functions, windows specific code
**
****************************************************************/

#include <stdio.h>
#include <stdlib.h>

#define DBG_USERMODE
#define DBG_PROGNAME "OPENCBM-XUM1541.DLL"
#include "debug.h"

#define OPENCBM_PLUGIN
#include "archlib.h"

#include "xum1541.h"


/*! \brief PARBURST: Read from the parallel port

 This function is a helper function for parallel burst:
 It reads from the parallel port.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \return
   The value read from the parallel port

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

unsigned char CBMAPIDECL
opencbm_plugin_parallel_burst_read(CBM_FILE HandleDevice)
{
    unsigned char result;

    result = (unsigned char)xum1541_ioctl((usb_dev_handle *)HandleDevice, XUM1541_PARBURST_READ, 0, 0);
    //printf("parburst read: %x\n", result);
    return result;
}

/*! \brief PARBURST: Write to the parallel port

 This function is a helper function for parallel burst:
 It writes to the parallel port.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Value
   The value to be written to the parallel port

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

void CBMAPIDECL
opencbm_plugin_parallel_burst_write(CBM_FILE HandleDevice, unsigned char Value)
{
    int result;

    result = xum1541_ioctl((usb_dev_handle *)HandleDevice, XUM1541_PARBURST_WRITE, Value, 0);
    //printf("parburst write: %x, res %x\n", Value, result);
}

int CBMAPIDECL
opencbm_plugin_parallel_burst_read_n(CBM_FILE HandleDevice, unsigned char *Buffer,
    unsigned int Length)
{
    int result;

    result = xum1541_read((usb_dev_handle *)HandleDevice, XUM1541_NIB_COMMAND, Buffer, Length);
    if (result != Length) {
        DBG_WARN((DBG_PREFIX "parallel_burst_read_n: returned with error %d", result));
    }

    return result;
}

int CBMAPIDECL
opencbm_plugin_parallel_burst_write_n(CBM_FILE HandleDevice, unsigned char *Buffer,
    unsigned int Length)
{
    int result;

    result = xum1541_write((usb_dev_handle *)HandleDevice, XUM1541_NIB_COMMAND, Buffer, Length);
    if (result != Length) {
        DBG_WARN((DBG_PREFIX "parallel_burst_write_n: returned with error %d", result));
    }

    return result;
}

/*! \brief PARBURST: Read a complete track

 This function is a helper function for parallel burst:
 It reads a complete track from the disk

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Buffer
   Pointer to a buffer which will hold the bytes read.

 \param Length
   The length of the Buffer.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL
opencbm_plugin_parallel_burst_read_track(CBM_FILE HandleDevice, unsigned char *Buffer, unsigned int Length)
{
    int result;

    result = xum1541_read((usb_dev_handle *)HandleDevice, XUM1541_NIB, Buffer, Length);
    if (result != Length) {
        DBG_WARN((DBG_PREFIX "parallel_burst_read_track: returned with error %d", result));
    }

    return result;
}

/*! \brief PARBURST: Read a variable length track

 This function is a helper function for parallel burst:
 It reads a variable length track from the disk

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Buffer
   Pointer to a buffer which will hold the bytes read.

 \param Length
   The length of the Buffer.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL
opencbm_plugin_parallel_burst_read_track_var(CBM_FILE HandleDevice, unsigned char *Buffer, unsigned int Length)
{
    int result;

    // Add a flag to indicate this read terminates early after seeing 
    // an 0x55 byte.
    result = xum1541_read((usb_dev_handle *)HandleDevice, XUM1541_NIB, Buffer, Length | XUM1541_NIB_READ_VAR);
    if (result <= 0) {
        DBG_WARN((DBG_PREFIX "parallel_burst_read_track_var: returned with error %d", result));
    }

    return result;
}

/*! \brief PARBURST: Write a complete track

 This function is a helper function for parallel burst:
 It writes a complete track to the disk

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Buffer
   Pointer to a buffer which hold the bytes to be written.

 \param Length
   The length of the Buffer.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL
opencbm_plugin_parallel_burst_write_track(CBM_FILE HandleDevice, unsigned char *Buffer, unsigned int Length)
{
    int result;

    result = xum1541_write((usb_dev_handle *)HandleDevice, XUM1541_NIB, Buffer, Length);
    if (result != Length) {
        DBG_WARN((DBG_PREFIX "parallel_burst_write_track: returned with error %d", result));
    }

    return result;
}

/********* Fast serial nibbler routines below ********/

/*! \brief SRQBURST: Read from the fast serial port

 This function is a helper function for fast serial burst:
 It reads from the fast serial port.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \return
   The value read from the fast serial port

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

unsigned char CBMAPIDECL
opencbm_plugin_srq_burst_read(CBM_FILE HandleDevice)
{
    unsigned char result;

    result = (unsigned char)xum1541_ioctl((usb_dev_handle *)HandleDevice, XUM1541_SRQBURST_READ, 0, 0);
    return result;
}

/*! \brief SRQBURST: Write to the fast serial port

 This function is a helper function for fast serial burst:
 It writes to the fast serial port.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Value
   The value to be written to the fast serial port

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

void CBMAPIDECL
opencbm_plugin_srq_burst_write(CBM_FILE HandleDevice, unsigned char Value)
{
    int result;

    result = xum1541_ioctl((usb_dev_handle *)HandleDevice, XUM1541_SRQBURST_WRITE, Value, 0);
}

int CBMAPIDECL
opencbm_plugin_srq_burst_read_n(CBM_FILE HandleDevice, unsigned char *Buffer,
    unsigned int Length)
{
    int result;

    result = xum1541_read((usb_dev_handle *)HandleDevice, XUM1541_NIB_SRQ_COMMAND, Buffer, Length);
    if (result != Length) {
        DBG_WARN((DBG_PREFIX "srq_burst_read_n: returned with error %d", result));
    }

    return result;
}

int CBMAPIDECL
opencbm_plugin_srq_burst_write_n(CBM_FILE HandleDevice, unsigned char *Buffer,
    unsigned int Length)
{
    int result;

    result = xum1541_write((usb_dev_handle *)HandleDevice, XUM1541_NIB_SRQ_COMMAND, Buffer, Length);
    if (result != Length) {
        DBG_WARN((DBG_PREFIX "srq_burst_write_n: returned with error %d", result));
    }

    return result;
}

/*! \brief SRQ: Read a complete track

 This function is a helper function for fast serial burst:
 It reads a complete track from the disk

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Buffer
   Pointer to a buffer which will hold the bytes read.

 \param Length
   The length of the Buffer.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL
opencbm_plugin_srq_burst_read_track(CBM_FILE HandleDevice, unsigned char *Buffer, unsigned int Length)
{
    int result;

    result = xum1541_read((usb_dev_handle *)HandleDevice, XUM1541_NIB_SRQ, Buffer, Length);
    if (result != Length) {
        DBG_WARN((DBG_PREFIX "srq_read_track: returned with error %d", result));
    }

    return result;
}

/*! \brief SRQ: Write a complete track

 This function is a helper function for fast serial burst:
 It writes a complete track to the disk

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Buffer
   Pointer to a buffer which hold the bytes to be written.

 \param Length
   The length of the Buffer.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL
opencbm_plugin_srq_burst_write_track(CBM_FILE HandleDevice, unsigned char *Buffer, unsigned int Length)
{
    int result;

    result = xum1541_write((usb_dev_handle *)HandleDevice, XUM1541_NIB_SRQ, Buffer, Length);
    if (result != Length) {
        DBG_WARN((DBG_PREFIX "srq_write_track: returned with error %d", result));
    }

    return result;
}

/**************** Tape routines below ****************/

/*! \brief TAPE: Prepare capture

 This function is a helper function for tape:
 It prepares the ZoomFloppy hardware for tape capture.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
*/

int CBMAPIDECL
opencbm_plugin_tap_prepare_capture(CBM_FILE HandleDevice, int *Status)
{
    *Status = xum1541_ioctl((usb_dev_handle *)HandleDevice, XUM1541_TAP_PREPARE_CAPTURE, 0, 0);
    //printf("opencbm_plugin_tap_prepare_capture: %x\n", result);
    return 1;
}

/*! \brief TAPE: Prepare write

 This function is a helper function for tape:
 It prepares the ZoomFloppy hardware for tape write.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
*/

int CBMAPIDECL
opencbm_plugin_tap_prepare_write(CBM_FILE HandleDevice, int *Status)
{
    *Status = xum1541_ioctl((usb_dev_handle *)HandleDevice, XUM1541_TAP_PREPARE_WRITE, 0, 0);
    //printf("opencbm_plugin_tap_prepare_write: %x\n", result);
    return 1;
}

/*! \brief TAPE: Get tape sense

 This function is a helper function for tape:
 It returns the current tape sense state.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \return
   The tape sense state

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
*/

int CBMAPIDECL
opencbm_plugin_tap_get_sense(CBM_FILE HandleDevice, int *Status)
{
    *Status = xum1541_ioctl((usb_dev_handle *)HandleDevice, XUM1541_TAP_GET_SENSE, 0, 0);
    //printf("opencbm_plugin_tap_get_sense: %x\n", result);
    return 1;
}

/*! \brief TAPE: Wait for <STOP> sense

 This function is a helper function for tape:
 It waits until the user stops the tape.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
*/

int CBMAPIDECL
opencbm_plugin_tap_wait_for_stop_sense(CBM_FILE HandleDevice, int *Status)
{
    *Status = xum1541_ioctl((usb_dev_handle *)HandleDevice, XUM1541_TAP_WAIT_FOR_STOP_SENSE, 0, 0);
    //printf("opencbm_plugin_tap_wait_for_stop_sense: %x\n", result);
    return 1;
}

/*! \brief TAPE: Wait for <PLAY> sense

 This function is a helper function for tape:
 It waits until the user presses play on tape.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
*/

int CBMAPIDECL
opencbm_plugin_tap_wait_for_play_sense(CBM_FILE HandleDevice, int *Status)
{
    *Status = xum1541_ioctl((usb_dev_handle *)HandleDevice, XUM1541_TAP_WAIT_FOR_PLAY_SENSE, 0, 0);
    //printf("opencbm_plugin_tap_wait_for_play_sense: %x\n", result);
    return 1;
}

/*! \brief TAPE: Motor on

 This function is a helper function for tape:
 It turns the tape drive motor on.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
*/

int CBMAPIDECL
opencbm_plugin_tap_motor_on(CBM_FILE HandleDevice, int *Status)
{
    *Status = xum1541_ioctl((usb_dev_handle *)HandleDevice, XUM1541_TAP_MOTOR_ON, 0, 0);
    //printf("opencbm_plugin_tap_motor_on: %x\n", result);
    return 1;
}

/*! \brief TAPE: Motor off

 This function is a helper function for tape:
 It turns the tape drive motor off.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
*/

int CBMAPIDECL
opencbm_plugin_tap_motor_off(CBM_FILE HandleDevice, int *Status)
{
    *Status = xum1541_ioctl((usb_dev_handle *)HandleDevice, XUM1541_TAP_MOTOR_OFF, 0, 0);
    //printf("opencbm_plugin_tap_motor_off: %x\n", result);
    return 1;
}

/*! \brief TAPE: Start capture

 This function is a helper function for tape:
 It starts the actual tape capture.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Buffer
   Pointer to a buffer which hold the bytes to be written.

 \param Buffer_Length
   The length of the Buffer.

 \param Status
   The return status.

 \param BytesRead
   The number of bytes read.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
*/

int CBMAPIDECL
opencbm_plugin_tap_start_capture(CBM_FILE HandleDevice, unsigned char *Buffer, unsigned int Buffer_Length, int *Status, int *BytesRead)
{
    int result = xum1541_read_ext((usb_dev_handle *)HandleDevice, XUM1541_TAP, Buffer, Buffer_Length, Status, BytesRead);
    if (result <= 0) {
        DBG_WARN((DBG_PREFIX "opencbm_plugin_tap_start_capture: returned with error %d", result));
    }
    return result;
}

/*! \brief TAPE: Start write

 This function is a helper function for tape:
 It starts the actual tape write.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Buffer
   Pointer to a buffer which holds the bytes to be written.

 \param Length
   The number of bytes to write.

 \param Status
   The return status.

 \param BytesWritten
   The number of bytes written.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
*/

int CBMAPIDECL
opencbm_plugin_tap_start_write(CBM_FILE HandleDevice, unsigned char *Buffer, unsigned int Length, int *Status, int *BytesWritten)
{
    int result = xum1541_write_ext((usb_dev_handle *)HandleDevice, XUM1541_TAP, Buffer, Length, Status, BytesWritten);
    if (result <= 0) {
        DBG_WARN((DBG_PREFIX "opencbm_plugin_tap_start_write: returned with error %d", result));
    }
    return result;
}

/*! \brief TAPE: Return tape firmware version

 This function is a helper function for tape:
 It returns the tape firmware version.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
*/

int CBMAPIDECL
opencbm_plugin_tap_get_ver(CBM_FILE HandleDevice, int *Status)
{
    *Status = xum1541_ioctl((usb_dev_handle *)HandleDevice, XUM1541_TAP_GET_VER, 0, 0);
    //printf("opencbm_plugin_tap_get_ver: %x\n", result);
    return 1;
}

int CBMAPIDECL
opencbm_plugin_tap_break(CBM_FILE HandleDevice)
{
    return xum1541_tap_break((usb_dev_handle *)HandleDevice);
    //printf("opencbm_plugin_tap_break: %x\n", result);
}

/*! \brief TAPE: Download configuration

 This function is a helper function for tape:
 It reads the active configuration.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Buffer
   Pointer to a buffer which hold the bytes to be written.

 \param Buffer_Length
   The length of the Buffer.

 \param Status
   The return status.

 \param BytesRead
   The number of bytes read.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
*/

int CBMAPIDECL
opencbm_plugin_tap_download_config(CBM_FILE HandleDevice, unsigned char *Buffer, unsigned int Buffer_Length, int *Status, int *BytesRead)
{
    int result = xum1541_read_ext((usb_dev_handle *)HandleDevice, XUM1541_TAP_CONFIG, Buffer, Buffer_Length, Status, BytesRead);
    if (result <= 0) {
        DBG_WARN((DBG_PREFIX "opencbm_plugin_tap_download_config: returned with error %d", result));
    }
    return result;
}

/*! \brief TAPE: Upload configuration

 This function is a helper function for tape:
 It writes the active configuration.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Buffer
   Pointer to a buffer which holds the bytes to be written.

 \param Length
   The number of bytes to write.

 \param Status
   The return status.

 \param BytesWritten
   The number of bytes written.

 \return
   != 0 on success.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.

 Note that a plugin is not required to implement this function.
*/

int CBMAPIDECL
opencbm_plugin_tap_upload_config(CBM_FILE HandleDevice, unsigned char *Buffer, unsigned int Length, int *Status, int *BytesWritten)
{
    int result = xum1541_write_ext((usb_dev_handle *)HandleDevice, XUM1541_TAP_CONFIG, Buffer, Length, Status, BytesWritten);
    if (result <= 0) {
        DBG_WARN((DBG_PREFIX "opencbm_plugin_tap_upload_config: returned with error %d", result));
    }
    return result;
}
