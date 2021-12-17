/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2004 Spiro Trikaliotis
*/

/*! **************************************************************
** \file sys/vdd/dll/vdd.h \n
** \author Spiro Trikaliotis \n
** \n
** \brief Function prototypes for the VDD
**
****************************************************************/

#ifndef VDD_H
#define VDD_H

#include <opencbm.h>

/*! the different function codes recognised by the VDD */
typedef
enum FUNCTIONCODE
{
    FC_DRIVER_OPEN,          /*!< call vdd_driver_open() */
    FC_DRIVER_CLOSE,         /*!< call vdd_driver_close() */
    FC_LISTEN,               /*!< call vdd_listen() */
    FC_TALK,                 /*!< call vdd_talk() */
    FC_OPEN,                 /*!< call vdd_open() */
    FC_CLOSE,                /*!< call vdd_close() */
    FC_RAW_READ,             /*!< call vdd_raw_read() */
    FC_RAW_WRITE,            /*!< call vdd_raw_write() */
    FC_UNLISTEN,             /*!< call vdd_unlisten() */
    FC_UNTALK,               /*!< call vdd_untalk() */
    FC_GET_EOI,              /*!< call vdd_get_eoi() */
    FC_CLEAR_EOI,            /*!< call vdd_clear_eoi() */
    FC_RESET,                /*!< call vdd_reset() */
    FC_PP_READ,              /*!< call vdd_pp_read() */
    FC_PP_WRITE,             /*!< call vdd_pp_write() */
    FC_IEC_POLL,             /*!< call vdd_iec_poll() */
    FC_IEC_GET,              /*!< call vdd_iec_get() */
    FC_IEC_SET,              /*!< call vdd_iec_set() */
    FC_IEC_RELEASE,          /*!< call vdd_iec_release() */
    FC_IEC_WAIT,             /*!< call vdd_iec_wait() */
    FC_UPLOAD,               /*!< call vdd_upload() */
    FC_DEVICE_STATUS,        /*!< call vdd_device_status() */
    FC_EXEC_COMMAND,         /*!< call vdd_exec_command() */
    FC_IDENTIFY,             /*!< call vdd_identify() */
    FC_GET_DRIVER_NAME,      /*!< call vdd_get_driver_name() */
    FC_VDD_INSTALL_IOHOOK,   /*!< call vdd_install_iohook() */
    FC_VDD_UNINSTALL_IOHOOK, /*!< call vdd_uninstall_iohook() */
    FC_VDD_USLEEP,           /*!< call vdd_usleep() */
    FC_IEC_SETRELEASE,       /*!< call vdd_setrelease() */
    FC_IDENTIFY_XP1541       /*!< call vdd_identify_xp1541() */
} FUNCTIONCODE;

extern HANDLE vdd_handle;

extern BOOLEAN vdd_driver_open(VOID);
extern BOOLEAN vdd_driver_close(CBM_FILE);
extern BOOLEAN vdd_raw_write(CBM_FILE);
extern BOOLEAN vdd_raw_read(CBM_FILE);
extern BOOLEAN vdd_listen(CBM_FILE);
extern BOOLEAN vdd_talk(CBM_FILE);
extern BOOLEAN vdd_open(CBM_FILE);
extern BOOLEAN vdd_close(CBM_FILE);
extern BOOLEAN vdd_unlisten(CBM_FILE);
extern BOOLEAN vdd_untalk(CBM_FILE);
extern BOOLEAN vdd_get_eoi(CBM_FILE);
extern BOOLEAN vdd_clear_eoi(CBM_FILE);
extern BOOLEAN vdd_reset(CBM_FILE);
extern BOOLEAN vdd_pp_read(CBM_FILE);
extern BOOLEAN vdd_pp_write(CBM_FILE);
extern BOOLEAN vdd_iec_poll(CBM_FILE);
extern BOOLEAN vdd_iec_set(CBM_FILE);
extern BOOLEAN vdd_iec_release(CBM_FILE);
extern BOOLEAN vdd_iec_setrelease(CBM_FILE);
extern BOOLEAN vdd_iec_wait(CBM_FILE);
extern BOOLEAN vdd_iec_get(CBM_FILE);
extern BOOLEAN vdd_upload(CBM_FILE);
extern BOOLEAN vdd_device_status(CBM_FILE);
extern BOOLEAN vdd_exec_command(CBM_FILE);
extern BOOLEAN vdd_identify(CBM_FILE);
extern BOOLEAN vdd_identify_xp1541(CBM_FILE);
extern BOOLEAN vdd_get_driver_name(VOID);

extern BOOLEAN vdd_install_iohook(CBM_FILE);
extern BOOLEAN vdd_uninstall_iohook(CBM_FILE);
extern USHORT  vdd_uninstall_iohook_internal(VOID);

extern BOOLEAN vdd_usleep(VOID);

extern CBM_FILE vdd_cbmfile_get(WORD);
extern WORD     vdd_cbmfile_store(CBM_FILE);
extern CBM_FILE vdd_cbmfile_delete(WORD);
extern VOID     vdd_cbmfile_closeall(VOID);

#endif /* #ifndef VDD_H */
