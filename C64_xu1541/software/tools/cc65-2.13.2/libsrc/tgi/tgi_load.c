/*****************************************************************************/
/*                                                                           */
/*                                tgi_load.c                                 */
/*                                                                           */
/*                       Loader module for TGI drivers                       */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/* (C) 2002-2009, Ullrich von Bassewitz                                      */
/*                Roemerstrasse 52                                           */
/*                D-70794 Filderstadt                                        */
/* EMail:         uz@cc65.org                                                */
/*                                                                           */
/*                                                                           */
/* This software is provided 'as-is', without any expressed or implied       */
/* warranty.  In no event will the authors be held liable for any damages    */
/* arising from the use of this software.                                    */
/*                                                                           */
/* Permission is granted to anyone to use this software for any purpose,     */
/* including commercial applications, and to alter it and redistribute it    */
/* freely, subject to the following restrictions:                            */
/*                                                                           */
/* 1. The origin of this software must not be misrepresented; you must not   */
/*    claim that you wrote the original software. If you use this software   */
/*    in a product, an acknowledgment in the product documentation would be  */
/*    appreciated but is not required.                                       */
/* 2. Altered source versions must be plainly marked as such, and must not   */
/*    be misrepresented as being the original software.                      */
/* 3. This notice may not be removed or altered from any source              */
/*    distribution.                                                          */
/*                                                                           */
/*****************************************************************************/



#include <tgi.h>
#include <tgi/tgi-kernel.h>



/*****************************************************************************/
/*                                   Code                                    */
/*****************************************************************************/



void __fastcall__ tgi_load (unsigned char mode)
/* Install the matching driver for the given mode. Will just load the driver
 * and check if loading was successul. Will not switch to gaphics mode.
 */
{
    const char* name = tgi_map_mode (mode);
    if (name == 0) {
        /* No driver for this mode */
        tgi_error = TGI_ERR_NO_DRIVER;
    } else {
        /* Load the driver */
        tgi_load_driver (name);
    }
}



