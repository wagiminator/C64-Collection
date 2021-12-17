/*
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2005 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Copyright 2001-2005 Spiro Trikaliotis
 *  Copyright 2011      Wolfgang Moser (http://d81.de)
 *  Copyright 2011      Thomas Winkler
 *
*/

/*! ************************************************************** 
** \file lib/detect.c \n
** \author Michael Klein, Spiro Trikaliotis \n
** \n
** \brief Shared library / DLL for accessing the driver
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


/*! \brief Identify the connected floppy drive.

 This function tries to identify a connected floppy drive.
 For this, it performs some M-R operations.

 \param HandleDevice
   A CBM_FILE which contains the file handle of the driver.

 \param DeviceAddress
   The address of the device on the IEC serial bus. This
   is known as primary address, too.

 \param CbmDeviceType
   Pointer to an enum which will hold the type of the device.

 \param CbmDeviceString
   Pointer to a pointer which will point on a string which
   tells the name of the device.

 \return
   0 if the drive could be contacted. It does not mean that
   the device could be identified.

 If cbm_driver_open() did not succeed, it is illegal to 
 call this function.
*/

int CBMAPIDECL 
cbm_identify(CBM_FILE HandleDevice, unsigned char DeviceAddress,
             enum cbm_device_type_e *CbmDeviceType, 
             const char **CbmDeviceString)
{
    enum cbm_device_type_e deviceType = cbm_dt_unknown;
    unsigned short magic;
    unsigned char buf[3];
    char command[] = { 'M', '-', 'R', (char) 0x40, (char) 0xff, (char) 0x02 };
    static char unknownDevice[] = "*unknown*, footprint=<....>";
    char *deviceString = unknownDevice;
    int rv = -1;

    FUNC_ENTER();

    /* get footprint from 0xFF40 */
    if (cbm_exec_command(HandleDevice, DeviceAddress, command, sizeof(command)) == 0 
        && cbm_talk(HandleDevice, DeviceAddress, 15) == 0)
    {
        if (cbm_raw_read(HandleDevice, buf, 3) == 3)
        {
            magic = buf[0] | (buf[1] << 8);

            if(magic == 0xaaaa)
            {
                cbm_untalk(HandleDevice);
                command[3] = (char) 0xFE; /* get footprint from 0xFFFE, IRQ vector */
                if (cbm_exec_command(HandleDevice, DeviceAddress, command, sizeof(command)) == 0 
                    && cbm_talk(HandleDevice, DeviceAddress, 15) == 0)
                {
                    if (cbm_raw_read(HandleDevice, buf, 3) == 3
                        && ( buf[0] != 0x67 || buf[1] != 0xFE ) )
                    {
                        magic = buf[0] | (buf[1] << 8);
                    }
                }
            }

            switch(magic)
            {
                default:
                    unknownDevice[22] = ((magic >> 12 & 0x0F) | 0x40);
                    unknownDevice[24] = ((magic >>  4 & 0x0F) | 0x40);
                    magic &= 0x0F0F;
                    magic |= 0x4040;
                    unknownDevice[23] = magic >> 8;
                    unknownDevice[25] = (char)magic;
                    break;

                case 0xaaaa:
                    deviceType = cbm_dt_cbm1541;
                    deviceString = "1540 or 1541"; 
                    break;

                case 0xf00f:
                    deviceType = cbm_dt_cbm1541;
                    deviceString = "1541-II";
                    break;

                case 0xcd18:
                    deviceType = cbm_dt_cbm1541;
                    deviceString = "1541C";
                    break;

                case 0x10ca:
                    deviceType = cbm_dt_cbm1541;
                    deviceString = "DolphinDOS 1541";
                    break;

                case 0x6f10:
                    deviceType = cbm_dt_cbm1541;
                    deviceString = "SpeedDOS 1541";
                    break;

                case 0x2710:
                    deviceType = cbm_dt_cbm1541;
                    deviceString = "ProfessionalDOS 1541";
                    break;

                case 0x8085:
                    deviceType = cbm_dt_cbm1541;
                    deviceString = "JiffyDOS 1541";
                    break;

                case 0xaeea:
                    deviceType = cbm_dt_cbm1541;
                    deviceString = "64'er DOS 1541";
                    break;

                case 0xfed7:
                    deviceType = cbm_dt_cbm1570;
                    deviceString = "1570";
                    break;

                case 0x02ac:
                    deviceType = cbm_dt_cbm1571;
                    deviceString = "1571";
                    break;

                case 0x01ba:
                    deviceType = cbm_dt_cbm1581;
                    deviceString = "1581";
                    break;

                case 0x32f0: 
                    deviceType = cbm_dt_cbm3040;
                    deviceString = "3040";
                    break;

                case 0xc320: 
                case 0x20f8: 
                    deviceType = cbm_dt_cbm4040;
                    deviceString = "4040";
                    break;

                case 0xf2e9:
                    deviceType = cbm_dt_cbm8050;
                    deviceString = "8050 dos2.5";
                    break;

                case 0xc866:       /* special dos2.7 ?? Speed-DOS 8250 ?? */
                case 0xc611:
                    deviceType = cbm_dt_cbm8250;
                    deviceString = "8250 dos2.7";
                    break;
            }
            rv = 0;
        }
        cbm_untalk(HandleDevice);
    }

    if(CbmDeviceType)
    {
        *CbmDeviceType = deviceType;
    }

    if(CbmDeviceString) 
    {
        *CbmDeviceString = deviceString;
    }

    FUNC_LEAVE_INT(rv);
}
