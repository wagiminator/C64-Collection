/*
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *  Copyright 2005 Spiro Trikaliotis
 *  Based on code by Wolfgang Moser
 *
*/

/*! ************************************************************** 
** \file lib/detectxp1541.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Shared library / DLL for accessing the driver
**        Detect an XP1541/XP1571 parallel cable
**
****************************************************************/

/*! Mark: We are in user-space (for debug.h) */
#define DBG_USERMODE

/*! The name of the executable */
#define DBG_PROGNAME "OPENCBM.DLL"

#include "debug.h"

#include <stdlib.h>

//! mark: We are building the DLL */
#define DLL
#include "opencbm.h"
#include "archlib.h"

#include "arch.h"


/*! \brief \internal Set the PIA back to input mode

 This function sets the parallel port PIA back to input mode.
 This prevents both PC and drive driving the lines, which
 could result in a hardware defect!

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Drive
   The address of the device on the IEC serial bus. This
   is known as primary address, too.

 \param PiaAddress
   The address in the drive's address space where the PIA, VIA
   or CIA output register for the parallel port is suspected.
   This routine assume that at PiaAddress + 2, there is the
   DDR of this port.

 \return
   0 if the drive could be contacted.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

static int
pia_to_inputmode(CBM_FILE HandleDevice, unsigned char Drive, unsigned int PiaAddress)
{
    int ret = -1;
    unsigned char byteval = 0x00;

    FUNC_ENTER();

    if (cbm_upload(HandleDevice, Drive, PiaAddress + 2, &byteval, 1) == 1)
        ret = 0;
    else
        ret = 1;

    FUNC_LEAVE_INT(ret);
}

/*! \brief \internal Output via floppy PIA and check if that can be read back

 This function outputs some byte via the floppy PIA and checks if
 that value can be read back using the parallel port of the PC.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param Drive
   The address of the device on the IEC serial bus. This
   is known as primary address, too.

 \param PiaAddress
   The address in the drive's address space where the PIA, VIA
   or CIA output register for the parallel port is suspected.
   This routine assume that at PiaAddress + 2, there is the
   DDR of this port.

 \param Value
   The value to write into the parallel port for testing.

 \return
   0 if the value could be set and read back.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

static int
output_pia(CBM_FILE HandleDevice, unsigned char Drive, unsigned int PiaAddress, unsigned char Value)
{
    int ret = -1;

    FUNC_ENTER();

    do {
        unsigned char byteval = 0xff;

        /*
         * Set the PIA port to output
         */
        if (cbm_upload(HandleDevice, Drive, PiaAddress + 2, &byteval, 1) != 1)
            break;

        /*
         * Output the value via the PIA port
         */
        if (cbm_upload(HandleDevice, Drive, PiaAddress , &Value, 1) != 1)
            break;

        /*
         * Give the drive time to be able to execute this command
         */
        arch_usleep(10000);

        /*
         * Read back the value. Hopefully, it is exactly what we just have written.
         */
        ret = cbm_pp_read(HandleDevice) != Value;

        if (ret)
        {
            /*
             * The test was unsuccessfull. To make sure the PIA
             * is not stuck at output mode, set it back to input mode.
             */
            pia_to_inputmode(HandleDevice, Drive, PiaAddress);
        }

    } while (0);

    FUNC_LEAVE_INT(ret);
}

/*! \brief Identify the cable connected to a specific floppy drive.

 This function tries to identify if the given floppy drive has an
 XP1541 cable connected.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param DeviceAddress
   The address of the device on the IEC serial bus. This
   is known as primary address, too.

 \param CbmDeviceType
   Pointer to an enum which holds the type of the device.
   If this pointer is NULL or the device type is set to
   unknown, this function calls cbm_identify itself to find
   out the device type. If this pointer is not set to NULL,
   this function will return the device type there.

 \param CableType
   Pointer to an enum which will hold the cable type of the 
   device on return.

 \return
   0 if the drive could be contacted. It does not mean that
   the device could be identified.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL 
cbm_identify_xp1541(CBM_FILE HandleDevice, unsigned char DeviceAddress,
                    enum cbm_device_type_e *CbmDeviceType,
                    enum cbm_cable_type_e *CableType)
{
    int ret = 0;
    enum cbm_device_type_e localDummyDeviceType = cbm_dt_unknown;

    FUNC_ENTER();
 
    do
    {
        unsigned int piaAddress;
        int value;

        if (!CableType)
        {
            /*
             * We do not have a possibility to send back the
             * cable type: This is an error!
             */
            DBG_ASSERT(("CableType is NULL", 0));
            ret = 1;
            break;
        }

        *CableType = cbm_ct_none;

        /*
         * If needed, determine the type of the drive
         */
        if (!CbmDeviceType)
            CbmDeviceType = &localDummyDeviceType;

        if (*CbmDeviceType == cbm_dt_unknown)
        {
            ret = cbm_identify(HandleDevice, DeviceAddress,
                 CbmDeviceType, NULL);
        }

        if (ret)
        {
            /*
             * We could not even determine the device type;
             * we do not dare trying to determine the cable
             * type, as writing into wrong locations can
             * make the drive hang.
             */
            *CableType = cbm_ct_unknown;
            break;
        }

        /*
         * Now that we know the drive type, use this to find the PIA/VIA/CIA address
         */
        switch (*CbmDeviceType)
        {
        case cbm_dt_cbm1541:
            piaAddress = 0x1801;
            break;

        case cbm_dt_cbm1570:
        case cbm_dt_cbm1571:
            piaAddress = 0x4001;
            break;

        default:
            piaAddress = 0;
            break;
        }

        /*
         * If we do not have a PIA address, we do not know where a parallel
         * cable could be located. This, report "no parallel cable".
         */
        if (piaAddress == 0)
            break;

        /*
         * Set parallel port into input mode.
         * This prevents us (PC) and the drive driving the lines simultaneously.
         */
        value = cbm_pp_read(HandleDevice);
        DBG_PRINT((DBG_PREFIX "Setting Parport into input mode", value));

        /*
         * Try to write some patterns and check if we see them:
         */
        if (output_pia(HandleDevice, DeviceAddress, piaAddress, 0x55))
            break;

        if (output_pia(HandleDevice, DeviceAddress, piaAddress, 0xAA))
            break;

        /*
         * Ok, it has worked: We have a parallel cable.
         */
        *CableType = cbm_ct_xp1541;

        /*
         * Set PIA back to input mode.
         * Again, this prevents PC and drive driving the lines simultaneously
         * (in the future).
         */
        if (piaAddress)
        {
            pia_to_inputmode(HandleDevice, DeviceAddress, piaAddress);
        }

    } while (0);

    FUNC_LEAVE_INT(ret);
}
