/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2004-2006 Spiro Trikaliotis
 *
 */

/*! ************************************************************** 
** \file sys/wdm/PortAccessWdm.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Functions for communicating with the parallel port driver
**        Only use WDM functions
**
****************************************************************/

#include <initguid.h>
#include <wdm.h>
#include "cbm_driver.h"

/*! \brief Set the operational mode of the parallel port

 This function sets the operational mode of the parallel port.

 \param Pdx
   Pointer to a device extension which contains the DEVICE_OBJECT 
   of the parallel port driver.

 This function has to be balanced with a corresponding ParPortUnsetMode()

 This function must be run at IRQL == PASSIVE_LEVEL.
*/
NTSTATUS
ParPortSetMode(PDEVICE_EXTENSION Pdx)
{
    FUNC_ENTER();

    FUNC_LEAVE_NTSTATUS(ParPortSetModeWdm(Pdx));
}

/*! \brief Unset the operational mode of the parallel port

 This function unsets the operational mode of the parallel port.

 \param Pdx
   Pointer to a device extension which contains the DEVICE_OBJECT 
   of the parallel port driver.

 This function mustn't be called without a prior call to
 ParPortSetMode()

 This function must be run at IRQL == PASSIVE_LEVEL.
*/
NTSTATUS
ParPortUnsetMode(PDEVICE_EXTENSION Pdx)
{
    FUNC_ENTER();

    FUNC_LEAVE_NTSTATUS(ParPortUnsetModeWdm(Pdx));
}
