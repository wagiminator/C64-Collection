/*
 *  CBM 1530/1531 tape routines.
 *  Copyright 2012 Arnd Menge, arnd(at)jonnz(dot)de
*/

#include <Windows.h>
#include <stdio.h>

#include "tape.h"

__int32 OutputError(__int32 Status)
{
	switch (Status)
	{
		case Tape_Status_ERROR:
			printf("General error.\n");
			break;
		case Tape_Status_ERROR_Device_Disconnected:
			printf("ZoomTape device was disconnected.\n");
			printf("Please disconnect and reconnect ZoomFloppy to continue.\n");
			break;
		case Tape_Status_ERROR_Device_Not_Configured:
			printf("Device not configured for tape operations.\n");
			break;
		case Tape_Status_ERROR_Sense_Not_On_Record:
			printf("Tape drive not on <RECORD>.\n");
			break;
		case Tape_Status_ERROR_Sense_Not_On_Play:
			printf("Tape drive not on <PLAY>.\n");
			break;
		case Tape_Status_ERROR_Write_Interrupted_By_Stop:
			printf("Write interrupted by <STOP>.\n");
			break;
		case Tape_Status_ERROR_usbSendByte:
			printf("usbSendByte failed.\n");
			break;
		case Tape_Status_ERROR_usbRecvByte:
			printf("usbRecvByte failed.\n");
			break;
		case Tape_Status_ERROR_External_Break:
			printf("External break.\n");
			break;
		case Tape_Status_ERROR_Wrong_Tape_Firmware:
			printf("Wrong tape firmware version.\n");
			break;
		default:
			// printf("Unknown error: %d\n", Status);
			return -1;
	}
	return 0;
}


__int32 OutputFuncError(__int32 Status)
{
	switch (Status)
	{
		case XUM1541_Error_NoTapeSupport:
			printf("No tape support. Update ZoomFloppy firmware.\n");
			break;
		case XUM1541_Error_NoDiskTapeMode:
			printf("ZoomFloppy not correctly initialized.\n");
			printf("Disconnect ZoomFloppy from USB and reconnect.\n");
			break;
		case XUM1541_Error_TapeCmdInDiskMode:
			printf("ZoomTape not detected at ZoomFloppy start.\n");
			printf("Disconnect ZoomFloppy from USB and reconnect.\n");
			break;
		default:
			// printf("Unknown error: %d\n", Status);
			return -1;
	}
	return 0;
}
