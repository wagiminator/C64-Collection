/*
 *  CBM 1530/1531 tape routines.
 *  Copyright 2012 Arnd Menge, arnd(at)jonnz(dot)de
*/

#include <stdio.h>
#include <stdlib.h>

#include <opencbm.h>
#include <arch.h>
#include "tape.h"
#include "misc.h"

// Global variables
unsigned __int8 MotorOn  = 0,
                MotorOff = 0;


void usage(void)
{
	printf("Usage: tapcontrol <command>\n");
	printf("\n");
	printf("Available commands:\n\n");
	printf("  on : Turn tape drive motor on\n");
	printf("  off: Turn tape drive motor off\n");
	printf("\n");
	printf("Examples:\n");
	printf("  tapcontrol on\n");
	printf("  tapcontrol off\n");
}


__int32 EvaluateCommandlineParams(__int32 argc, __int8 *argv[])
{
	if (argc != 2)
		return -1;

	argv++;

	// Evaluate commandline flag.
	if (strcmp(*argv,"on") == 0)
	{
	   	printf("* Turning tape motor on.\n");
	   	MotorOn = 1;
	}
	else if (strcmp(*argv,"off") == 0)
	{
	   	printf("* Turning tape motor off.\n");
	   	MotorOff = 1;
	}
	else
		return -1;

	return 0;
}


int ARCH_MAINDECL main(int argc, char *argv[])
{
	CBM_FILE fd;
	__int32  Status, FuncRes, RetVal = 0;

	printf("\ntapcontrol v1.00 - Commodore 1530/1531 tape control\n");
	printf("Copyright 2012 Arnd Menge\n\n");

	if ((RetVal = EvaluateCommandlineParams(argc, argv)) == -1)
	{
		usage();
		goto exit;
	}

	if (cbm_driver_open_ex(&fd, NULL) != 0)
	{
		printf("\nDriver error.\n");
		RetVal = -1;
		goto exit;
	}

	// Check tape firmware version compatibility.
	//   Status value:
	//   - tape firmware version
	//   - XUM1541_Error_NoTapeSupport
	//   - XUM1541_Error_NoDiskTapeMode
	//   - XUM1541_Error_TapeCmdInDiskMode
	FuncRes = cbm_tap_get_ver(fd, &Status);
	if (FuncRes != 1)
	{
		printf("\nReturned error [get_ver]: %d\n", FuncRes);
		RetVal = -1;
		goto exit2;
	}
	if (Status < 0)
	{
		printf("\nReturned error [get_ver]: ");
		if (OutputError(Status) < 0)
			if (OutputFuncError(Status) < 0)
				printf("%d\n", Status);
		RetVal = -1;
		goto exit2;
	}
	if (Status != TapeFirmwareVersion)
	{
		printf("\nError [get_ver]: ");
		OutputError(Tape_Status_ERROR_Wrong_Tape_Firmware);
		RetVal = -1;
		goto exit2;
	}

	if (MotorOn == 1)
	{
		//   Status values:
		//   - Tape_Status_OK_Motor_On
		//   - Tape_Status_ERROR_Device_Disconnected
		//   - XUM1541_Error_NoTapeSupport
		//   - XUM1541_Error_NoDiskTapeMode
		//   - XUM1541_Error_TapeCmdInDiskMode
		FuncRes = cbm_tap_motor_on(fd, &Status);
		if (FuncRes != 1)
		{
			printf("\nReturned error [motor_on]: %d\n", FuncRes);
			RetVal = -1;
		}
		if (Status != Tape_Status_OK_Motor_On)
		{
			printf("\nReturned error [motor_on]: ");
			if (OutputError(Status) < 0)
				if (OutputFuncError(Status) < 0)
					printf("%d\n", Status);
			RetVal = -1;
		}
	}

	if (MotorOff == 1)
	{
		//   Status values:
		//   - Tape_Status_OK_Motor_Off
		//   - Tape_Status_ERROR_Device_Disconnected
		//   - XUM1541_Error_NoTapeSupport
		//   - XUM1541_Error_NoDiskTapeMode
		//   - XUM1541_Error_TapeCmdInDiskMode
		FuncRes = cbm_tap_motor_off(fd, &Status);
		if (FuncRes != 1)
		{
			printf("\nReturned error [motor_off]: %d\n", FuncRes);
			RetVal = -1;
		}
		if (Status != Tape_Status_OK_Motor_Off)
		{
			printf("\nReturned error [motor_off]: ");
			if (OutputError(Status) < 0)
				if (OutputFuncError(Status) < 0)
					printf("%d\n", Status);
			RetVal = -1;
		}
	}

    exit2:
	cbm_driver_close(fd);

    exit:
   	printf("\n");
   	return RetVal;
}
