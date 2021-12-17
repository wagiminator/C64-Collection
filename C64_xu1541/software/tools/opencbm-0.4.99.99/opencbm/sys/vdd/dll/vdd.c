/*
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *  Copyright 2004 Spiro Trikaliotis
*/

/*! ************************************************************** 
** \file sys/vdd/dll/vdd.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief VDD for accessing the driver from DOS
**
****************************************************************/

// #define UNICODE 1

#include <windows.h>
#include <vddsvc.h>

#include <opencbm.h>

/*! Mark: We are in user-space (for debug.h) */
#define DBG_USERMODE

/*! Mark: We are building a DLL */
#define DBG_DLL

/*! The name of the executable */
#define DBG_PROGNAME "OPENCBMVDD.DLL"

/*! This file is "like" debug.c, that is, define some variables */
#define DBG_IS_DEBUG_C

#include "debug.h"

#include "vdd.h"

#include <stdlib.h>


/*! we are exporting the functions */
#undef EXTERN
#define EXTERN __declspec(dllexport) 

EXTERN BOOL VDDInitialize(IN HANDLE Module, IN DWORD Reason, IN LPVOID Reserved);
EXTERN VOID VDDRegisterInit(VOID);
EXTERN VOID VDDDispatch(VOID);


/*! The handle of the vdd (for being accessed by the I/O hook install functions */
HANDLE vdd_handle;

/*! \brief VDD blocking callback

 Whenever the VDD is blocked (that is, there is no active DOS program
 left), this function is called. This opportunity is used to close
 all active CBM_FILE handles.
*/

static VOID
VDDBlockHandler(VOID)
{
    FUNC_ENTER();

    vdd_cbmfile_closeall();

    FUNC_LEAVE();
}

/*! \brief VDD initialization und unloading

 This function is called whenever the VDD is loaded or unloaded.
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
*/

BOOL
VDDInitialize(IN HANDLE Module, IN DWORD Reason, IN LPVOID Reserved)
{
    FUNC_ENTER();

    DBG_PRINT((DBG_PREFIX "OpencbmVDD.Entry: " __DATE__ " " __TIME__));

    switch (Reason) 
    {
        case DLL_PROCESS_ATTACH:
            vdd_handle = Module;
            VDDInstallUserHook(vdd_handle, NULL, NULL, VDDBlockHandler, NULL);
            break;

        case DLL_PROCESS_DETACH:
            DBG_ASSERT(vdd_handle == Module);
            vdd_uninstall_iohook_internal();
            VDDDeInstallUserHook(Module);

            // make sure all CBM_FILE handles are closed
            // whenever this VDD is unloaded
            vdd_cbmfile_closeall();
            break;

        default:
            break;

    }

    FUNC_LEAVE_BOOL(TRUE);
}

/*! \brief RegisterModule processing

 This function is called when the DOS portion calls the
 REGISTERMODULE function.
*/

VOID
VDDRegisterInit(VOID)
{
    FUNC_ENTER();

    setCF(0);

    FUNC_LEAVE();
}

/*! \brief RegisterModule processing

 This function is called when the DOS portion calls the
 DISPATCHCALL function.
*/

VOID
VDDDispatch(VOID)
{
    FUNCTIONCODE functioncode;
    CBM_FILE cbmfile;
    BOOLEAN error;

    FUNC_ENTER();

    functioncode = getDL();

    error = FALSE;

    // convert to BX value into a CBM_FILE
    switch (functioncode)
    {
    case FC_VDD_USLEEP:
    case FC_DRIVER_OPEN:
    case FC_GET_DRIVER_NAME:
        // FC_VDD_USLEEP, FC_DRIVER_OPEN and FC_GET_DRIVER_NAME are special,
        // they do not have a BX input.
        break;

    default:
        cbmfile = vdd_cbmfile_get(getBX());

        if (cbmfile == INVALID_HANDLE_VALUE)
        {
            DBG_ERROR((DBG_PREFIX "invalid BX given: %04x", getBX()));
            error = TRUE;
        }
        break;
    }

    if (!error)
    {
        switch (functioncode)
        {
        case FC_DRIVER_OPEN:     error = vdd_driver_open();          break;
        case FC_DRIVER_CLOSE:    error = vdd_driver_close(cbmfile);  break;
        case FC_LISTEN:          error = vdd_listen(cbmfile);        break;
        case FC_TALK:            error = vdd_talk(cbmfile);          break;
        case FC_OPEN:            error = vdd_open(cbmfile);          break;
        case FC_CLOSE:           error = vdd_close(cbmfile);         break;
        case FC_RAW_READ:        error = vdd_raw_read(cbmfile);      break;
        case FC_RAW_WRITE:       error = vdd_raw_write(cbmfile);     break;
        case FC_UNLISTEN:        error = vdd_unlisten(cbmfile);      break;
        case FC_UNTALK:          error = vdd_untalk(cbmfile);        break;
        case FC_GET_EOI:         error = vdd_get_eoi(cbmfile);       break;
        case FC_CLEAR_EOI:       error = vdd_clear_eoi(cbmfile);     break;
        case FC_RESET:           error = vdd_reset(cbmfile);         break;
        case FC_PP_READ:         error = vdd_pp_read(cbmfile);       break;
        case FC_PP_WRITE:        error = vdd_pp_write(cbmfile);      break;
        case FC_IEC_POLL:        error = vdd_iec_poll(cbmfile);      break;
        case FC_IEC_GET:         error = vdd_iec_get(cbmfile);       break;
        case FC_IEC_SET:         error = vdd_iec_set(cbmfile);       break;
        case FC_IEC_RELEASE:     error = vdd_iec_release(cbmfile);   break;
        case FC_IEC_SETRELEASE:  error = vdd_iec_setrelease(cbmfile); break;
        case FC_IEC_WAIT:        error = vdd_iec_wait(cbmfile);      break;
        case FC_UPLOAD:          error = vdd_upload(cbmfile);        break;
        case FC_DEVICE_STATUS:   error = vdd_device_status(cbmfile); break; 
        case FC_EXEC_COMMAND:    error = vdd_exec_command(cbmfile);  break;
        case FC_IDENTIFY:        error = vdd_identify(cbmfile);      break;
        case FC_IDENTIFY_XP1541: error = vdd_identify_xp1541(cbmfile); break;
        case FC_GET_DRIVER_NAME: error = vdd_get_driver_name();      break;

        case FC_VDD_USLEEP:      error = vdd_usleep();               break;

        case FC_VDD_INSTALL_IOHOOK:   error = vdd_install_iohook(cbmfile);   break;
        case FC_VDD_UNINSTALL_IOHOOK: error = vdd_uninstall_iohook(cbmfile); break;

        default:
            // this function is not implemented:
            DBG_ERROR((DBG_PREFIX "unknown function code in DL: %02x", functioncode));
            error = TRUE;
            break;
        }
    }

    setCF(error ? 1 : 0);

    FUNC_LEAVE();
}
