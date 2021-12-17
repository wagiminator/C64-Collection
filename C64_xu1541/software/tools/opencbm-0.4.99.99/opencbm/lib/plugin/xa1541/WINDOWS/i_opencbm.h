/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2004 Spiro Trikaliotis
 *
 */

/*! **************************************************************
** \file lib/plugin/xa1541/WINDOWS/i_opencbm.h \n
** \author Spiro Trikaliotis \n
** \n
** \brief Internal API for opencbm installation
**
****************************************************************/

#ifndef I_OPENCBM_H
#define I_OPENCBM_H

#include "configuration.h"
#include "opencbm.h"

extern BOOL cbm_ioctl(IN CBM_FILE HandleDevice, IN DWORD ControlCode,
#if DBG
          IN char *TextControlCode,
#endif // #if DBG
          IN PVOID InBuffer, IN ULONG InBufferSize,
          OUT PVOID OutBuffer, IN ULONG OutBufferSize);

/*! A macro for the call to cbm_ioctl()
 * Remember, I'm lazy...
 */

#if DBG
    #define CBMCTRL( _x_ ) CBMCTRL_##_x_, "CBMCTRL_" #_x_
#else  // #if DBG
    #define CBMCTRL( _x_ ) CBMCTRL_##_x_
#endif // #if DBG

extern BOOL cbm_driver_stop(VOID);
extern BOOL cbm_driver_start(VOID);

extern BOOL cbm_driver_install(OUT PULONG Buffer, IN ULONG BufferLen);

extern LONG RegGetDWORD(IN HKEY RegKey, IN char *SubKey, OUT LPDWORD Value);
extern BOOL IsDriverStartedAutomatically(VOID);

extern VOID WaitForIoCompletionInit(VOID);
extern VOID WaitForIoCompletionDeinit(VOID);
extern VOID WaitForIoCompletionCancelAll(VOID);
extern VOID WaitForIoCompletionConstruct(LPOVERLAPPED Overlapped);
extern BOOL WaitForIoCompletion(BOOL Result, CBM_FILE HandleDevice, LPOVERLAPPED Overlapped, DWORD *BytesTransferred);

/*! Registry key where the opencbm driver is located (under HKLM) */
#define REGKEY_EVENTLOG \
            "System\\CurrentControlSet\\Services\\Eventlog\\System\\opencbm"

extern PCHAR FormatErrorMessage(DWORD Error);
extern BOOL CbmInstall(IN LPCTSTR DriverName, IN LPCTSTR ServiceExe, IN BOOL AutomaticStart);

extern BOOL CbmRemove(IN LPCTSTR DriverName);
extern BOOL CbmCheckPresence(IN LPCTSTR DriverName);

extern int CbmCheckDriver(void);


extern VOID CbmParportRestart(VOID);

extern BOOL CbmUpdateParameter(IN ULONG DefaultLpt,
                               IN ULONG IecCableType,
                               IN ULONG PermanentlyLock,
                               IN BOOL DebugFlagsDriverPresent, IN ULONG DebugFlagsDriver,
                               IN BOOL DebugFlagsDllPresent, IN ULONG DebugFlagsDll);

#if DBG
extern VOID CbmOutputDebuggingBuffer(VOID);
#endif

#endif /* I_OPENCBM_H */
