// This demo file only works with the memory models:
// TINY, SMALL.
// Other's may work, but are not tested.
// Essentially, this program assumes that all data is accessed
// via DS, and that DS, ES and SS are always the same.


/*
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *  Copyright 2004 Spiro Trikaliotis
*/

/*! ************************************************************** 
** \file sample/dos/c/opencbn.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Library for accessing the driver from DOS
**
****************************************************************/

#include "opencbm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned int WORD;


#define RegisterModule   db 0xc4, 0xc4, 0x58, 0x00
#define UnRegisterModule db 0xc4, 0xc4, 0x58, 0x01
#define DispatchCall     db 0xc4, 0xc4, 0x58, 0x02

#define CbmDispatchCallNoBx(_no_) \
     mov dl,_no_; mov ax,[VDD_HANDLE]; DispatchCall

/*
#define CbmDispatchCall(_no_) \
     mov bx,[f]; mov dl,_no_; mov ax,[VDD_HANDLE]; DispatchCall
*/
#define CbmDispatchCall(_no_) \
     mov bx,[f]; CbmDispatchCallNoBx(_no_)

#define CbmDispatchCallRetVal(_no_) \
        CbmDispatchCall(_no_); mov [retVal],ax


#define INVALID_HANDLE -1

static WORD VDD_HANDLE = INVALID_HANDLE;

static int vdd_initialized = 0;

int
vdd_init(void)
{
    static const char DllName[] = "OpencbmVDD.DLL";
    static const char InitFunc[] = "VDDRegisterInit";
    static const char DispFunc[] = "VDDDispatch";

    WORD error;

    if (vdd_initialized)
    {
        return 0;
    }

    asm {
        push es

        push ds
        pop es

        lea si,[word ptr DllName] /* ds:si */
        lea di,[InitFunc]  /* es:di */
        lea bx,[DispFunc]  /* ds:bx */

        RegisterModule

        mov [VDD_HANDLE],ax

        /* make sure we notice of an error occurred */
        mov ax,0
        adc ax,0
        mov [error],ax

        pop es
    }

    if (error)
    {
        switch (VDD_HANDLE)
        {
        case 1: printf("vdd_init(): DLL not found!\n"); break;
        case 2: printf("vdd_init(): Dispatch routine not found!\n"); break;
        case 3: printf("vdd_init(): Init routine not found!\n"); break;
        case 4: printf("vdd_init(): Insufficient memory!\n"); break;
        default: printf("vdd_init(): unknown error reason: %u\n", VDD_HANDLE); break;
        }

        VDD_HANDLE = INVALID_HANDLE;
    }
    else
    {
        atexit(vdd_uninit);
        vdd_initialized = 1;
    }

    return error ? 1 : 0;
}

void
vdd_uninit(void)
{
    if (vdd_initialized)
    {
        vdd_initialized = 0;

        asm {
            mov ax,[VDD_HANDLE]
            UnRegisterModule
        }

        VDD_HANDLE = INVALID_HANDLE;
    }
}

int
cbm_driver_open(CBM_FILE *f, int port)
{
    CBM_FILE cbmfile;
    WORD retax;

    vdd_init();

    if (VDD_HANDLE != INVALID_HANDLE && port < 256)
    {
        asm {
            mov dh,byte ptr [port]
            CbmDispatchCallNoBx(0)
            mov [cbmfile],BX
            mov [retax],AX
        }
        *f = cbmfile;
    }
    else
    {
        retax = 1;
    }

    return retax;
}


void
cbm_driver_close(CBM_FILE f)
{
    asm {
        CbmDispatchCall(1)
    }
}


int
cbm_listen(CBM_FILE f, unsigned char dev, unsigned char secadr)
{
    WORD retVal;

    asm {
        mov ch,[dev]
        mov cl,[secadr]
        CbmDispatchCallRetVal(2)
    }
    return retVal;
}

int
cbm_talk(CBM_FILE f, unsigned char dev, unsigned char secadr)
{
    WORD retVal;

    asm {
        mov ch,[dev]
        mov cl,[secadr]
        CbmDispatchCallRetVal(3)
    }
    return retVal;
}

int
cbm_open(CBM_FILE f, unsigned char dev, unsigned char secadr, const void *fname, size_t len)
{
    WORD retVal;

    asm {
        mov ch,[dev]
        mov cl,[secadr]
        mov si,[fname]
        mov di,[len]
        CbmDispatchCallRetVal(4)
    }
    return retVal;
}

int
cbm_close(CBM_FILE f, unsigned char dev, unsigned char secadr)
{
    WORD retVal;

    asm {
        mov ch,[dev]
        mov cl,[secadr]
        CbmDispatchCallRetVal(5)
    }
    return retVal;
}


int
cbm_raw_read(CBM_FILE f, void *buf, size_t size)
{
    WORD retVal;

    asm {
        mov si,[buf]
        mov cx,[size]
        CbmDispatchCallRetVal(6)
    }
    return retVal;
}


int
cbm_raw_write(CBM_FILE f, const void *buf, size_t size)
{
    WORD retVal;

    asm {
        mov si,[buf]
        mov cx,[size]
        CbmDispatchCallRetVal(7)
    }
    return retVal;
}


int
cbm_unlisten(CBM_FILE f)
{
    WORD retVal;

    asm {
        CbmDispatchCallRetVal(8)
    }
    return retVal;
}

int
cbm_untalk(CBM_FILE f)
{
    WORD retVal;

    asm {
        CbmDispatchCallRetVal(9)
    }
    return retVal;
}

int
cbm_get_eoi(CBM_FILE f)
{
    WORD retVal;

    asm {
        CbmDispatchCallRetVal(10)
    }
    return retVal;
}

// @@@untested
int
cbm_clear_eoi(CBM_FILE f)
{
    WORD retVal;

    asm {
        CbmDispatchCallRetVal(11)
    }
    return retVal;
}


int
cbm_reset(CBM_FILE f)
{
    WORD retVal;

    asm {
        CbmDispatchCallRetVal(12)
    }
    return retVal;
}


// @@@untested
unsigned char
cbm_pp_read(CBM_FILE f)
{
    WORD retVal;

    asm {
        CbmDispatchCallRetVal(13)
    }
    return retVal;
}

// @@@untested
void
cbm_pp_write(CBM_FILE f, unsigned char c)
{
    asm {
        mov cl,[c]
        CbmDispatchCall(14)
    }
}

// @@@untested
int
cbm_iec_poll(CBM_FILE f)
{
    WORD retVal;

    asm {
        CbmDispatchCallRetVal(15)
    }
    return retVal;
}

// @@@untested
int
cbm_iec_get(CBM_FILE f, int line)
{
    WORD retVal;

    asm {
        mov cl,byte ptr [line]
        CbmDispatchCallRetVal(16)
    }
    return retVal;
}

void
cbm_iec_set(CBM_FILE f, int line)
{
    asm {
        mov cl,byte ptr [line]
        CbmDispatchCall(17)
    }
}

void
cbm_iec_release(CBM_FILE f, int line)
{
    asm {
        mov cl,byte ptr [line]
        CbmDispatchCall(18)
    }
}

// @@@untested
int
cbm_iec_wait(CBM_FILE f, int line, int state)
{
    WORD retVal;

    asm {
        mov cl,byte ptr [line]
        mov ch,byte ptr [state]
        CbmDispatchCallRetVal(19)
    }
    return retVal;
}


int
cbm_upload(CBM_FILE f, unsigned char dev, int adr, const void *prog, size_t size)
{
    WORD retVal;

    asm {
        mov dh,[dev]
        mov di,[adr]
        mov si,[prog]
        mov cx,[size]
        CbmDispatchCallRetVal(20)
    }
    return retVal;
}


int
cbm_device_status(CBM_FILE f, unsigned char dev, void *buf, size_t bufsize)
{
    WORD retVal;

    asm {
        mov dh,[dev]
        mov si,[buf]
        mov cx,[bufsize]
        CbmDispatchCallRetVal(21)
    }
    return retVal;
}

int
cbm_exec_command(CBM_FILE f, unsigned char dev, const void *cmd, size_t len)
{
    WORD retVal;

    asm {
        mov dh,[dev]
        mov si,[cmd]
        mov cx,[len]
        CbmDispatchCallRetVal(22)
    }
    return retVal;
}


int
cbm_identify(CBM_FILE f, unsigned char drv,
                                   enum cbm_device_type_e *t,
                                   const char **type_str)
{
    WORD retVal;
    static char local_type_str[80];
    enum cbm_device_type_e local_devicetype;

    asm {
        mov dh,[drv]
        mov si,offset [local_type_str]
        mov cx,80
        CbmDispatchCallRetVal(23)
        mov [local_devicetype],di
    }
    if (t)
    {
        *t = local_devicetype;
    }
    if (type_str)
    {
        *type_str = local_type_str;
    }
    return retVal;
}


#define GETDRIVERNAME_PORTNAME_MAXLENGTH 256
static char GetDriverName_Portname[GETDRIVERNAME_PORTNAME_MAXLENGTH];

const char *
cbm_get_driver_name(int port)
{
    vdd_init();

    if (VDD_HANDLE != INVALID_HANDLE && port < 256)
    {
        asm {
            mov dh,byte ptr [port]
            lea si,[GetDriverName_Portname]
            mov cx,GETDRIVERNAME_PORTNAME_MAXLENGTH
            CbmDispatchCallNoBx(24)
        }

        GetDriverName_Portname[GETDRIVERNAME_PORTNAME_MAXLENGTH-1] = 0;
    }
    else
    {
        strcpy(GetDriverName_Portname, "<invalid port no!>");
    }

    return GetDriverName_Portname;
}

int
vdd_install_iohook(CBM_FILE f, int IoBaseAddress, int CableType)
{
    WORD retVal;

    asm {
        mov cx,[IoBaseAddress]
        mov dh,byte ptr [CableType]
        CbmDispatchCallRetVal(25)
    }
    return retVal;
}

int
vdd_uninstall_iohook(CBM_FILE f)
{
    WORD retVal;

    asm {
        CbmDispatchCallRetVal(26)
    }
    return retVal;
}

void
vdd_usleep(CBM_FILE f, unsigned int howlong)
{
    asm {
        mov cx,[howlong]
        CbmDispatchCall(27)
    }
}

void
cbm_iec_setrelease(CBM_FILE f, int set, int release)
{
    asm {
        mov ch,byte ptr [set]
        mov cl,byte ptr [release]
        CbmDispatchCall(28)
    }
}
