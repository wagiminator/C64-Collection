/*
 * USB configuration application for the xum1541
 * Copyright 2010 Nate Lawson <nate@root.org>
 * Copyright 2010 Wolfgang Moser
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

/*! **************************************************************
** \file lib/plugin/xum1541/usbcfg.c \n
** \author Nate Lawson \n
** \n
** \brief USB configuration application for the xum1541
****************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "opencbm.h"

#include "arch.h"
#include "archlib_ex.h"
#include "xum1541_types.h"


/*! \brief Initialize the xum1541 device
  This function tries to find and identify the xum1541 device.

  \return
    0 on success, 1 on error.
*/
int ARCH_MAINDECL
main(int argc, char **argv)
{
    int ret;
    CBM_FILE HandleXum1541 = NULL;

    ret = opencbm_plugin_init();
    if (ret == 0) {
        ret = opencbm_plugin_driver_open(&HandleXum1541, 0);
    }
    if (ret != 0) {
        fprintf(stderr, "initialization error, aborting\n");
        exit(1);
    }
    if (xum1541_plugin_control_msg(HandleXum1541, XUM1541_ENTER_BOOTLOADER) == 0) {
        fprintf(stderr, "Success. xum1541 now in bootloader mode.\n");
        fprintf(stderr, "Run your firmware update program now.\n");
    } else {
        fprintf(stderr, "error entering bootloader mode, aborting\n");
    }
    opencbm_plugin_driver_close(HandleXum1541);
    opencbm_plugin_uninit();
    exit(0);
}
