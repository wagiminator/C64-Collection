/*
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *  Copyright 2005      Tim Schürmann
 *  Copyright 2005,2007,2009 Spiro Trikaliotis
 *  Copyright 2009      Arnd <arnd(at)jonnz(dot)de>
 *
*/

/*! ************************************************************** 
** \file lib/plugin/xa1541/WINDOWS/parburst.c \n
** \author Tim Schürmann, Spiro Trikaliotis, Arnd \n
** \n
** \brief Shared library / DLL for accessing the mnib driver functions, windows specific code
**
****************************************************************/

#include <windows.h>
#include <windowsx.h>

#include <mmsystem.h>

/*! Mark: We are in user-space (for debug.h) */
#define DBG_USERMODE

/*! The name of the executable */
#define DBG_PROGNAME "OPENCBM-XA1541.DLL"

#include "debug.h"

#include <winioctl.h>
#include "cbmioctl.h"

#include <stdlib.h>

//! mark: We are building the DLL */

#include "i_opencbm.h"

#define OPENCBM_PLUGIN
#include "archlib.h"


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
    CBMT_PARBURST_PREAD_OUT result;

    FUNC_ENTER();

    cbm_ioctl(HandleDevice, CBMCTRL(PARBURST_READ), NULL, 0, &result, sizeof(result));

    FUNC_LEAVE_UCHAR(result.Byte);
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
    CBMT_PARBURST_PWRITE_IN parameter;

    FUNC_ENTER();

    parameter.Byte = Value;

    cbm_ioctl(HandleDevice, CBMCTRL(PARBURST_WRITE), &parameter, sizeof(parameter), NULL, 0);

    FUNC_LEAVE();
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
    int retval = 0;

    FUNC_ENTER();

    retval = cbm_ioctl(HandleDevice, CBMCTRL(PARBURST_READ_TRACK),
        NULL, 0,
        Buffer, Length);

    if (retval == 0)
    {
        DBG_WARN((DBG_PREFIX "opencbm: cbm.c: parallel_burst_read_track: ioctl returned with error %u", retval));
    }

    FUNC_LEAVE_INT(retval);
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
    int retval = 0;

    FUNC_ENTER();

    retval = cbm_ioctl(HandleDevice, CBMCTRL(PARBURST_READ_TRACK_VAR),
        NULL, 0,
        Buffer, Length);

    if (retval == 0)
    {
        DBG_WARN((DBG_PREFIX "opencbm: cbm.c: parallel_burst_read_track_var: ioctl returned with error %u", retval));
    }

    FUNC_LEAVE_INT(retval);
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
    int retval = 0;

    FUNC_ENTER();

    retval = cbm_ioctl(HandleDevice, CBMCTRL(PARBURST_WRITE_TRACK),
        Buffer, Length,
        NULL, 0);

    if (retval == 0)
    {
        DBG_WARN((DBG_PREFIX "opencbm: cbm.c: parallel_burst_write_track: ioctl returned with error %u", retval));
    }

    FUNC_LEAVE_INT(retval);
}
