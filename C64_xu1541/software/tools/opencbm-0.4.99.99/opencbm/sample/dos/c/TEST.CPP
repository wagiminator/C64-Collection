/*
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *  Copyright 2004 Spiro Trikaliotis
*/

/*! ************************************************************** 
** \file sample/dos/c/test.cpp \n
** \author Spiro Trikaliotis \n
** \n
** \brief Some tests for the DOS API
**
****************************************************************/

#include "opencbm.h"

#include <stdio.h>
#include <string.h>
#include <dos.h>

int
own_device_status(CBM_FILE HandleDevice, unsigned char DeviceAddress,
                  void *Buffer, size_t BufferLength)
{
    int retValue;

    // Pre-occupy return value

    retValue = 99;

    if (Buffer && (BufferLength > 0))
    {
        char *bufferToWrite = (char*) Buffer;

        // make sure we have a trailing zero at the end of the buffer:

        bufferToWrite[--BufferLength] = '\0';

        // pre-occupy buffer with the error value

        strncpy(bufferToWrite, "99, DRIVER ERROR,00,00\r", BufferLength);

        // Now, ask the drive for its error status:

        if (cbm_talk(HandleDevice, DeviceAddress, 15) == 0)
        {
            unsigned bytesRead = cbm_raw_read(HandleDevice, bufferToWrite, BufferLength);

            // make sure we have a trailing zero at the end of the status:

            bufferToWrite[bytesRead] = '\0';

            cbm_untalk(HandleDevice);
        }
        else
        {
            printf("cbm_talk returned an error!\n");
        }

        retValue = atoi(bufferToWrite);
    }

    return retValue;
}

void morse(CBM_FILE f, unsigned char drv)
{
    static char morsecode[] = {
 0x20,0xe5,0xc1,0xd0,0x0a,0xa9,0x30,0x4c,
 0xc8,0xc1,0xa2,0x08,0x20,0x4e,0x05,0xb1,
 0xa3,0xaa,0xe0,0x30,0x90,0xef,0xe0,0x3a,
 0xb0,0x05,0xbd,0x40,0x05,0xd0,0x0b,0xe0,
 0x41,0x90,0xe2,0xe0,0x5b,0xb0,0xde,0xbd,
 0x39,0x05,0x85,0x14,0x29,0x07,0x85,0x15,
 0xa2,0x01,0x06,0x14,0x90,0x02,0xa2,0x04,
 0x20,0x61,0x05,0xa2,0x01,0x20,0x68,0x05,
 0xc6,0x15,0xd0,0xec,0xc8,0xcc,0x74,0x02,
 0x90,0xc0,0x60,0x8d,0x00,0x1c,0x8a,0xa2,
 0x80,0x84,0x1f,0xa0,0x00,0xc8,0xd0,0xfd,
 0xe8,0xd0,0xfa,0x4a,0xd0,0xf7,0xa4,0x1f,
 0x60,0xad,0x00,0x1c,0x09,0x08,0xd0,0xe3,
 0xad,0x00,0x1c,0x29,0xf7,0x4c,0x4b,0x05,
 0xfd,0x7d,0x3d,0x1d,0x0d,0x05,0x85,0xc5,
 0xe5,0xf5,0x42,0x84,0xa4,0x83,0x01,0x24,
 0xc3,0x04,0x02,0x74,0xa3,0x44,0xc2,0x82,
 0xe3,0x64,0xd4,0x43,0x03,0x81,0x23,0x14,
 0x63,0x94,0xb4,0xc4
    };

    int n;

    n = cbm_upload(f, drv, 0x500, morsecode, sizeof(morsecode));
    printf("cbm_upload %s!\n",
       n == sizeof(morsecode) ? "success" : "FAILED");

    cbm_exec_command(f, drv, "U3:SOS", 6);
}

void
writefile(CBM_FILE f, unsigned char drv, const char * const filename)
{
    const char text[] = "Hello, just a simple Test!";

    int n;

    n = cbm_open(f, drv, 2, filename, 0);
    printf("cbm_open \"%s\" %s!\n", filename, n == 0 ? "success" : "FAILED");

    if (n == 0)
    {
        cbm_listen(f, drv, 2);
        cbm_raw_write(f, text, strlen(text));
        cbm_unlisten(f);

        n = cbm_close(f, drv, 2);
        printf("cbm_close %s!\n", n == 0 ? "success" : "FAILED");
    }

    printf("\n");
}

void
readfile(CBM_FILE f, unsigned char drv, const char * const filename)
{
    int n;

    n = cbm_open(f, drv, 2, filename, 0);
    printf("cbm_open \"%s\" %s!\n", filename, n == 0 ? "success" : "FAILED");

    if (n==0)
    {
        printf("File contents: '");
        cbm_talk(f, drv, 2);

        while (!cbm_get_eoi(f))
        {
            char c;

            cbm_raw_read(f, &c, 1);

            printf("%c", c);
        }
        printf("'\n");
        cbm_untalk(f);

        n = cbm_close(f, drv, 2);
        printf("cbm_close %s!\n", n == 0 ? "success" : "FAILED");
    }
    printf("\n");
}

void
changelines(CBM_FILE f)
{
    cbm_iec_release(f,IEC_DATA);
    cbm_iec_release(f,IEC_ATN);
    cbm_iec_release(f,IEC_CLOCK);

    sleep(2);

    for (int atn=0; atn<2; atn++)
    {
        if (atn)
            cbm_iec_set(f, IEC_ATN);
        else
            cbm_iec_release(f, IEC_ATN);

        for (int clk=0; clk<2; clk++)
        {
            if (clk)
                cbm_iec_set(f, IEC_CLOCK);
            else
                cbm_iec_release(f, IEC_CLOCK);

            for (int data=0; data<2; data++)
            {
                if (data)
                    cbm_iec_set(f, IEC_DATA);
                else
                    cbm_iec_release(f, IEC_DATA);

                sleep(1);
            }
        }
    }
}

int main(int argc, char **argv)
{
    int ret, n;
    unsigned char drive = 8;
    CBM_FILE f;
    char buffer[80];
    char *devicetype_str;
    const char *drivername;
    enum cbm_device_type_e devicetype;
    int checklines = 0;
    int checkiohook = 0;

    if (argc>1)
    {
        int i;

        for (i=0; i < argc; i++)
        {
            switch (argv[i][0])
            {
            case '-':
                    checklines = 1;
                    break;

            case '+':
                    checkiohook = 1;
                    break;

            default:
                    drive = atoi(argv[i]);
            }
        }
    }

    printf("VDDTEST " __DATE__ " " __TIME__ ", using drive %u\n", drive);

    if (vdd_init())
    {
        printf("Could not initialize the VDD, aborting!\n");
        exit(1);
    }

    drivername = cbm_get_driver_name(0);
    printf("cbm_get_driver_name() returned %s.\n", drivername);

    ret = cbm_driver_open(&f, 0);

    if (ret)
    {
        printf("cbm_driver_open FAILED!\n");
        exit(1);
    }
    printf("cbm_driver_open success!\n\n");

    n = cbm_reset(f);
    printf("cbm_reset %s\n", n==0 ? "success" : "FAILED");

    if (checklines)
    {
        changelines(f);
    }

    n = cbm_device_status(f, drive, buffer, sizeof(buffer));
    printf("cbm_device_status returned: %u\n%s\n\n", n, buffer);

    n = cbm_open(f, drive, 15, "I0", 0);
    printf("cbm_open %u,15,\"I0\" %s!\n", drive, n == 0 ? "success" : "FAILED");
    n = cbm_close(f, drive, 15);
    printf("cbm_close %s!\n", n == 0 ? "success" : "FAILED");

    n = own_device_status(f, drive, buffer, sizeof(buffer));
    printf("own_device_status returned: %u\n%s\n\n", n, buffer);

    n = cbm_identify(f, drive, &devicetype, &devicetype_str);
    printf("cbm_identify %s!\n", n==0 ? "success" : "FAILED");
    printf("cbm_identify returned: %u - '%s'.\n\n", (unsigned int) devicetype,
                         devicetype_str);

    n = cbm_exec_command(f, drive, "S0:OPENCBM.VDD", 0);
    printf("cbm_exec_command(scratch) %s!\n", n==0 ? "success" : "FAILED");

    writefile(f, drive, "OPENCBM.VDD,S,W");
    readfile(f, drive, "OPENCBM.VDD,S,R");

    morse(f, drive);

    if (checkiohook)
    {
        unsigned int base = 0x0fc0;

        printf("trying to install iohook at 0x%08x ", base);
        if (vdd_install_iohook(f, base, 1))
        {
            int i;

            printf("success\n");

            for (i=0; i<15; i++)
            {
                outportb(base + 2, i);
                sleep(1);
            }

            printf("trying to uninstall iohook ");
            if (vdd_uninstall_iohook(f))
            {
                printf("success\n");
            }
            else
            {
                printf("FAILED\n");
            }
        }
        else
        {
            printf("FAILED\n");
        }
    }

    cbm_driver_close(f);
    printf("cbm_driver_close success!\n");

    return 0;
}
