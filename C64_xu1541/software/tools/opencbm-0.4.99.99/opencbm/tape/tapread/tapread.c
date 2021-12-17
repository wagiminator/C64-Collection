/*
 *  CBM 1530/1531 tape routines.
 *  Copyright 2012 Arnd Menge, arnd(at)jonnz(dot)de
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include <opencbm.h>
#include <arch.h>
#include "cap.h"
#include "tape.h"
#include "misc.h"

// Global variables
unsigned __int8  CAP_Machine, CAP_Video, CAP_StartEdge, CAP_SignalFormat;
unsigned __int32 CAP_Precision, CAP_SignalWidth, CAP_StartOfs;
CBM_FILE         fd;

// Break handling variables
CRITICAL_SECTION CritSec_fd, CritSec_BreakHandler;
BOOL             fd_Initialized = FALSE, AbortTapeOps = FALSE;


void usage(void)
{
	printf("Usage: tapread <type> [buffer size] [sampling rate] <filename.cap>\n");
	printf("\n");
	printf("Please specify the tape type:\n\n");
	printf("  -c64pal : C64 PAL     \n");
	printf("  -c64ntsc: C64 NTSC    \n");
	printf("  -c16pal : C16 PAL     \n");
	printf("  -c16ntsc: C16 NTSC    \n");
	printf("  -vicpal : VIC-20 PAL  \n");
	printf("  -vicntsc: VIC-20 NTSC \n");
	printf("  -spec48k: Spectrum48K \n");
	printf("  -x      : custom/unknown\n");
	printf("\n");
	printf("You can specify the buffer size for the capture data (optional):\n\n");
	printf("  -b10 :  10 Megabyte\n");
	printf("  -b25 :  25 Megabyte (default)\n");
	printf("  -b50 :  50 Megabyte\n");
	printf("  -b100: 100 Megabyte\n");
	printf("\n");
	printf("You can specify the sampling rate (optional):\n\n");
	printf("  -s1 :  1 MHz (default)\n");
	printf("  -s16: 16 MHz (maximum precision)\n");
	printf("\n");
	printf("Examples:\n");
	printf("  tapread -c64pal myfile.cap\n");
	printf("  tapread -c64pal -b50 -s16 myfile.cap");
}


__int32 EvaluateCommandlineParams(__int32 argc, __int8 *argv[], __int32 *piTapeBufferSize, __int8 filename[_MAX_PATH])
{
	unsigned __int8 bTapeType = 0, bBufferSize = 0, bSamplingRate = 0; // Commandline flag counters.

	if ((argc < 3) || (5 < argc))
	{
		printf("Error: invalid number of commandline parameters.\n\n");
		return -1;
	}

	*piTapeBufferSize = 1024*1024*25; // Default buffer size: 25MB.

	// Evaluate flags.
	while (--argc && (*(++argv)[0] == '-'))
	{
		if (strcmp(*argv,"-c64pal") == 0)
		{
			printf("* Tape type: C64 PAL\n");
			CAP_Machine = CAP_Machine_C64;
			CAP_Video = CAP_Video_PAL;
			bTapeType++;
		}
		else if (strcmp(*argv,"-c64ntsc") == 0)
		{
			printf("* Tape type: C64 NTSC\n");
			CAP_Machine = CAP_Machine_C64;
			CAP_Video = CAP_Video_NTSC;
			bTapeType++;
		}
		else if (strcmp(*argv,"-c16pal") == 0)
		{
			printf("* Tape type: C16 PAL\n");
			CAP_Machine = CAP_Machine_C16;
			CAP_Video = CAP_Video_PAL;
			bTapeType++;
		}
		else if (strcmp(*argv,"-c16ntsc") == 0)
		{
			printf("* Tape type: C16 NTSC\n");
			CAP_Machine = CAP_Machine_C16;
			CAP_Video = CAP_Video_NTSC;
			bTapeType++;
		}
		else if (strcmp(*argv,"-vicpal") == 0)
		{
			printf("* Tape type: VIC PAL\n");
			CAP_Machine = CAP_Machine_VC20;
			CAP_Video = CAP_Video_PAL;
			bTapeType++;
		}
		else if (strcmp(*argv,"-vicntsc") == 0)
		{
			printf("* Tape type: VIC NTSC\n");
			CAP_Machine = CAP_Machine_VC20;
			CAP_Video = CAP_Video_NTSC;
			bTapeType++;
		}
		else if (strcmp(*argv,"-spec48k") == 0)
		{
			printf("* Tape type: Spectrum48K\n");
			CAP_Machine = CAP_Machine_Spec48K;
			CAP_Video = CAP_Video_Spec48K;
			bTapeType++;
		}
		else if ((*argv)[1] == 'x')
		{
			printf("* Tape type: custom/unknown\n");
			CAP_Machine = CAP_Machine_CUSTOM;
			CAP_Video = CAP_Video_CUSTOM;
			bTapeType++;
		}
		else if (strcmp(*argv,"-b10") == 0)
		{
			*piTapeBufferSize = 1024*1024*10;
			printf("* Buffer size: 10 MB\n");
			bBufferSize++;
		}
		else if (strcmp(*argv,"-b25") == 0)
		{
			*piTapeBufferSize = 1024*1024*25;
			printf("* Buffer size: 25 MB\n");
			bBufferSize++;
		}
		else if (strcmp(*argv,"-b50") == 0)
		{
			*piTapeBufferSize = 1024*1024*50;
			printf("* Buffer size: 50 MB\n");
			bBufferSize++;
		}
		else if (strcmp(*argv,"-b100") == 0)
		{
			*piTapeBufferSize = 1024*1024*100;
			printf("* Buffer size: 100 MB\n");
			bBufferSize++;
		}
		else if (strcmp(*argv,"-s1") == 0)
		{
			CAP_Precision = 1;
			printf("* Sampling rate: %d MHz\n", CAP_Precision);
			bSamplingRate++;
		}
		else if (strcmp(*argv,"-s16") == 0)
		{
			CAP_Precision = 16;
			printf("* Sampling rate: %d MHz\n", CAP_Precision);
			bSamplingRate++;
		}
		else
		{
			printf("\nError: invalid commandline parameter.\n\n");
			return -1;
		}
	}

	if (bTapeType == 0)
	{
		printf("\nError: <type> not specified.\n\n");
		return -1;
	}
	else if (bTapeType > 1)
	{
		printf("\nError: <type> specified more than once.\n\n");
		return -1;
	}

	if (bBufferSize == 0)
	{
		printf("* Buffer size: 25 MB\n"); // use default value
	}
	else if (bBufferSize > 1)
	{
		printf("\nError: [buffer size] specified more than once.\n\n");
		return -1;
	}

	if (bSamplingRate == 0)
	{
		printf("* Sampling rate: %d MHz\n", CAP_Precision); // use default value
	}
	else if (bSamplingRate > 1)
	{
		printf("\nError: [sampling rate] specified more than once.\n\n");
		return -1;
	}

	if (strlen(argv[0]) >= _MAX_PATH)
	{
		printf("\nError: Filename too long.\n\n");
		return -1;
	}

	strcpy(filename, argv[0]);

	return 0;
}


// Allocate memory for tape image.
__int32 AllocateImageBuffer(void **ppucTapeBuffer, __int32 iTapeBufferSize)
{
	if (iTapeBufferSize < 0)
	{
		printf("Error: Illegal tape image buffer size.\n");
		return -1;
	}

	// Allocate memory for tape image.
	*ppucTapeBuffer = malloc(iTapeBufferSize);
	if (*ppucTapeBuffer == NULL)
	{
		printf("Error: Could not allocate memory for capture data.\n");
		return -1;
	}

	// Initialize memory for tape image.
	memset(*ppucTapeBuffer, 0x00, iTapeBufferSize);

	return 0;
}


// Print tape length to console.
void OutputTapeLength(unsigned __int32 uiTotalTapeTimeSeconds, unsigned __int32 uiNumSignals, __int32 iCaptureLen)
{
	unsigned __int32 hours, mins, secs;

	hours = (uiTotalTapeTimeSeconds/3600);
	printf("Tape length: %uh", hours);
	mins = ((uiTotalTapeTimeSeconds - hours*3600)/60);
	printf(" %um", mins);
	secs = ((uiTotalTapeTimeSeconds - hours*3600) - mins*60);
	printf(" %us", secs);
	printf(" (%d bytes) (%d signals)\n", iCaptureLen, uiNumSignals);
}


// Convert timestamps to 5 bytes, downscale precision to 1us if requested and write to CAP file.
__int32 ConvertAndWriteCaptureData(HANDLE hCAP, unsigned __int8 *pucTapeBuffer, __int32 iCaptureLen, unsigned __int32 *puiTotalTapeTimeSeconds, unsigned __int32 *puiNumSignals)
{
	unsigned __int64 ui64Delta, ui64TotalTapeTime = 0;
	__int32          FuncRes, i = 0;

	*puiTotalTapeTimeSeconds = 0;
	*puiNumSignals = 0;

	while (i < iCaptureLen)
	{
		ui64Delta = pucTapeBuffer[i];
		ui64Delta = (ui64Delta << 8) + pucTapeBuffer[i+1];

		if (ui64Delta < 0x8000)
		{
			// Short signal (<2ms)
			i += 2;
		}
		else
		{
			// Long signal (>=2ms)
			ui64Delta &= 0x7fff;
			ui64Delta = (ui64Delta << 8) + pucTapeBuffer[i+2];
			ui64Delta = (ui64Delta << 8) + pucTapeBuffer[i+3];
			ui64Delta = (ui64Delta << 8) + pucTapeBuffer[i+4];
			i += 5;
		}

		ui64TotalTapeTime += ui64Delta;
		(*puiNumSignals)++;

		if (CAP_Precision == 1) ui64Delta = (ui64Delta + 8) >> 4; // downscale by 16

		FuncRes = CAP_WriteSignal(hCAP, ui64Delta, NULL);
		if (FuncRes != CAP_Status_OK)
		{
			CAP_OutputError(FuncRes);
			return -1;
		}
	}

	// Calculate tape length in seconds.
	*puiTotalTapeTimeSeconds = (unsigned __int32) (((ui64TotalTapeTime + 8000000) >> 10)/15625); //16000000;

	return 0;
}


// Write tape image to specified image file.
__int32 WriteTapeBufferToCaptureFile(HANDLE hCAP, unsigned __int8 *pucTapeBuffer, __int32 iCaptureLen)
{
	unsigned __int32 uiTotalTapeTimeSeconds, uiNumSignals;
	__int32          FuncRes;

	if (iCaptureLen == 0)
		printf("Empty capture file.\n");

	FuncRes = CAP_SetHeader(hCAP, CAP_Precision, CAP_Machine, CAP_Video, CAP_StartEdge, CAP_SignalFormat, CAP_SignalWidth, CAP_StartOfs);
	if (FuncRes != CAP_Status_OK)
	{
		CAP_OutputError(FuncRes);
		return -1;
	}

	FuncRes = CAP_WriteHeader(hCAP);
	if (FuncRes != CAP_Status_OK)
	{
		CAP_OutputError(FuncRes);
		return -1;
	}

	FuncRes = CAP_WriteHeaderAddon(hCAP, "   Created by       ZoomTape    ----------------", 0x30);
	if (FuncRes != CAP_Status_OK)
	{
		CAP_OutputError(FuncRes);
		return -1;
	}

	// Convert timestamps to 5 bytes, downscale precision to 1us if requested and write to CAP file.
	if (ConvertAndWriteCaptureData(hCAP, pucTapeBuffer, iCaptureLen, &uiTotalTapeTimeSeconds, &uiNumSignals) == -1)
		return -1;

	// Print tape length to console.
	OutputTapeLength(uiTotalTapeTimeSeconds, uiNumSignals, iCaptureLen);

	return 0;
}


__int32 CaptureTape(CBM_FILE fd, unsigned __int8 *pucTapeBuffer, __int32 iTapeBufferSize, __int32 *piCaptureLen)
{
	unsigned __int8 ReadConfig, ReadConfig2;
	__int32         Status, BytesRead, BytesWritten, FuncRes;

	// Check abort flag.
	if (AbortTapeOps)
		return -1;

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
		return -1;
	}
	if (Status < 0)
	{
		printf("\nReturned error [get_ver]: ");
		if (OutputError(Status) < 0)
			if (OutputFuncError(Status) < 0)
				printf("%d\n", Status);
		return -1;
	}
	if (Status != TapeFirmwareVersion)
	{
		printf("\nError [get_ver]: ");
		OutputError(Tape_Status_ERROR_Wrong_Tape_Firmware);
		return -1;
	}

	// Prepare tape read configuration.
	if (CAP_StartEdge == CAP_StartEdge_Falling)
		ReadConfig = (unsigned __int8)XUM1541_TAP_READ_STARTFALLEDGE; // Start reading with falling edge.
	else
		ReadConfig = ~(unsigned __int8)XUM1541_TAP_READ_STARTFALLEDGE; // Start reading with rising edge.

	// Check abort flag.
	if (AbortTapeOps)
		return -1;

	// Upload tape read configuration.
	//   Return values:
	//   - Tape_Status_OK_Config_Uploaded
	//   - Tape_Status_ERROR_usbRecvByte
	//   FuncRes values concerning tape mode:
	//   - XUM1541_Error_NoTapeSupport
	//   - XUM1541_Error_NoDiskTapeMode
	//   - XUM1541_Error_TapeCmdInDiskMode
	FuncRes = cbm_tap_upload_config(fd, &ReadConfig, 1, &Status, &BytesWritten);
	if (FuncRes < 0)
	{
		printf("\nReturned error [upload_config]: ");
		if (OutputFuncError(FuncRes) < 0)
			printf("%d\n", FuncRes);
		return -1;
	}
	if (Status != Tape_Status_OK_Config_Uploaded)
	{
		printf("\nReturned error [upload_config]: ");
		if (OutputError(Status) < 0)
			printf("%d\n", Status);
		return -1;
	}
	if (BytesWritten != 1)
	{
		printf("\nError [upload_config]: Invalid data size (%d).\n", BytesWritten);
		return -1;
	}

	// Check abort flag.
	if (AbortTapeOps)
		return -1;

	// Download tape read configuration and check.
	//   Return values:
	//   - Tape_Status_OK_Config_Downloaded
	//   - Tape_Status_ERROR_usbSendByte
	//   FuncRes values concerning tape mode:
	//   - XUM1541_Error_NoTapeSupport
	//   - XUM1541_Error_NoDiskTapeMode
	//   - XUM1541_Error_TapeCmdInDiskMode
	FuncRes = cbm_tap_download_config(fd, &ReadConfig2, 1, &Status, &BytesRead);
	if (FuncRes < 0)
	{
		printf("\nReturned error [download_config]: ");
		if (OutputFuncError(FuncRes) < 0)
			printf("%d\n", FuncRes);
		return -1;
	}
	if (Status != Tape_Status_OK_Config_Downloaded)
	{
		printf("\nReturned error [download_config]: ");
		if (OutputError(Status) < 0)
			printf("%d\n", Status);
		return -1;
	}
	if (BytesRead != 1)
	{
		printf("\nError [download_config]: Invalid data size (%d).\n", BytesRead);
		return -1;
	}
	if ((ReadConfig & 0x60) != (ReadConfig2 & 0x60))
	{
		printf("\nError [download_config]: Configuration mismatch.\n");
		return -1;
	}

	// Check abort flag.
	if (AbortTapeOps)
		return -1;

	// Prepare tape firmware for capture.
	//   Status values:
	//   - Tape_Status_OK_Device_Configured_for_Read
	//   - Tape_Status_ERROR_Device_Disconnected
	//   - Tape_Status_ERROR_Not_In_Tape_Mode
	//   FuncRes values concerning tape mode:
	//   - XUM1541_Error_NoTapeSupport
	//   - XUM1541_Error_NoDiskTapeMode
	//   - XUM1541_Error_TapeCmdInDiskMode
	FuncRes = cbm_tap_prepare_capture(fd, &Status);
	if (FuncRes < 0)
	{
		printf("\nReturned error [prepare_capture]: ");
		if (OutputFuncError(FuncRes) < 0)
			printf("%d\n", FuncRes);
		return -1;
	}
	if (Status != Tape_Status_OK_Device_Configured_for_Read)
	{
		printf("\nReturned error [prepare_capture]: ");
		if (OutputError(Status) < 0)
			printf("%d\n", Status);
		return -1;
	}

	// Check abort flag.
	if (AbortTapeOps)
		return -1;

	// Get tape SENSE state.
	//   Status values:
	//   - Tape_Status_OK_Sense_On_Play
	//   - Tape_Status_OK_Sense_On_Stop
	//   - Tape_Status_ERROR_Device_Not_Configured
	//   - Tape_Status_ERROR_Device_Disconnected
	//   - Tape_Status_ERROR_Not_In_Tape_Mode
	//   - XUM1541_Error_NoTapeSupport
	//   - XUM1541_Error_NoDiskTapeMode
	//   - XUM1541_Error_TapeCmdInDiskMode
	FuncRes = cbm_tap_get_sense(fd, &Status);
	if (FuncRes != 1)
	{
		printf("\nReturned error [get_sense]: %d\n", FuncRes);
		return -1;
	}
	if ((Status != Tape_Status_OK_Sense_On_Play) && (Status != Tape_Status_OK_Sense_On_Stop))
	{
		printf("\nReturned error [get_sense]: ");
		if (OutputError(Status) < 0)
			if (OutputFuncError(Status) < 0)
				printf("%d\n", Status);
		return -1;
	}

	if (Status == Tape_Status_OK_Sense_On_Play)
	{
		// Check abort flag.
		if (AbortTapeOps)
			return -1;

		printf("Please <STOP> the tape.\n");
		//   Status values:
		//   - Tape_Status_OK_Sense_On_Stop
		//   - Tape_Status_ERROR_Device_Not_Configured
		//   - Tape_Status_ERROR_Device_Disconnected
		//   - Tape_Status_ERROR_Not_In_Tape_Mode
		//   - XUM1541_Error_NoTapeSupport
		//   - XUM1541_Error_NoDiskTapeMode
		//   - XUM1541_Error_TapeCmdInDiskMode
		FuncRes = cbm_tap_wait_for_stop_sense(fd, &Status);
		if (FuncRes != 1)
		{
			printf("\nReturned error [wait_for_stop_sense]: %d\n", FuncRes);
			return -1;
		}
		if (Status != Tape_Status_OK_Sense_On_Stop)
		{
			printf("\nReturned error [wait_for_stop_sense]: ");
			if (OutputError(Status) < 0)
				if (OutputFuncError(Status) < 0)
					printf("%d\n", Status);
			return -1;
		}
	}

	// Check abort flag.
	if (AbortTapeOps)
		return -1;

	printf("Press <PLAY> on tape.\n");

	//   Status values:
	//   - Tape_Status_OK_Sense_On_Play
	//   - Tape_Status_ERROR_Device_Not_Configured
	//   - Tape_Status_ERROR_Device_Disconnected
	//   - Tape_Status_ERROR_Not_In_Tape_Mode
	//   - XUM1541_Error_NoTapeSupport
	//   - XUM1541_Error_NoDiskTapeMode
	//   - XUM1541_Error_TapeCmdInDiskMode
	FuncRes = cbm_tap_wait_for_play_sense(fd, &Status);
	if (FuncRes != 1)
	{
		printf("\nReturned error [wait_for_play_sense]: %d\n", FuncRes);
		return -1;
	}
	if (Status != Tape_Status_OK_Sense_On_Play)
	{
		printf("\nReturned error [wait_for_play_sense]: ");
		if (OutputError(Status) < 0)
			if (OutputFuncError(Status) < 0)
				printf("%d\n", Status);
		return -1;
	}

	// Check abort flag.
	if (AbortTapeOps)
		return -1;

	printf("\nReading tape...\n");

	//   Status values:
	//   - Tape_Status_OK_Capture_Finished
	//   - Tape_Status_ERROR_Sense_Not_On_Play
	//   - Tape_Status_ERROR_Device_Not_Configured
	//   - Tape_Status_ERROR_Device_Disconnected
	//   FuncRes values concerning tape mode:
	//   - XUM1541_Error_NoTapeSupport
	//   - XUM1541_Error_NoDiskTapeMode
	//   - XUM1541_Error_TapeCmdInDiskMode
	FuncRes = cbm_tap_start_capture(fd, pucTapeBuffer, iTapeBufferSize, &Status, &BytesRead);
	if (FuncRes < 0)
	{
		printf("\nReturned error [capture]: ");
		if (OutputFuncError(FuncRes) < 0)
			printf("%d\n", FuncRes);
		return -1;
	}
	*piCaptureLen = BytesRead;
	if (*piCaptureLen >= iTapeBufferSize)
	{
		printf("\nError [capture]: Buffer full, use larger buffer size!\n");
		return -1;
	}
	if (Status != Tape_Status_OK_Capture_Finished)
	{
		printf("\nReturned error [capture]: ");
		if (OutputError(Status) < 0)
			printf("%d\n", Status);
		return -1;
	}

	// Check abort flag.
	if (AbortTapeOps)
		return -1;

	printf("\nReading finished OK.\n");

	return 0;
}


// Break handler.
// Initialized by SetConsoleCtrlHandler() after critical sections initialized.
BOOL BreakHandler(DWORD fdwCtrlType)
{
	if (fdwCtrlType == CTRL_C_EVENT)
	{
		printf("\nAborting...\n");
		AbortTapeOps = TRUE; // Flag tape ops abort.

		if (!TryEnterCriticalSection(&CritSec_BreakHandler))
			return TRUE; // Already aborting.

		EnterCriticalSection(&CritSec_fd); // Acquire handle flag access.

		if (fd_Initialized)
		{
			cbm_tap_break(fd); // Handle valid.
			LeaveCriticalSection(&CritSec_fd); // Release handle flag access.
			return TRUE;
		}

		LeaveCriticalSection(&CritSec_fd); // Release handle flag access.
	}
	return FALSE;
}


// Main routine.
//   Return values:
//    0: tape reading finished ok
//   -1: an error occurred
int ARCH_MAINDECL main(int argc, char *argv[])
{
	HANDLE          hCAP;
	unsigned __int8 *pucTapeBuffer = NULL;
	__int8          filename[_MAX_PATH];
	__int32         iCaptureLen, iTapeBufferSize = 0;
	__int32         FuncRes, RetVal = -1;

	printf("\ntapread v1.00 - Commodore 1530/1531 tape image creator\n");
	printf("Copyright 2012 Arnd Menge\n\n");

	InitializeCriticalSection(&CritSec_fd);
	InitializeCriticalSection(&CritSec_BreakHandler);

	if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE) BreakHandler, TRUE))
		printf("Ctrl-C break handler not installed.\n\n");

	// Set defaults.
	CAP_Precision    = 1;                         // Default: 1us signal precision.
	CAP_StartEdge    = CAP_StartEdge_Falling;     // Default: Start with falling signal edge.
	CAP_SignalFormat = CAP_SignalFormat_Relative; // Default: Relative timings instead of absolute.
	CAP_SignalWidth  = CAP_SignalWidth_40bit;     // Default: 40bit.
	CAP_StartOfs     = CAP_Default_Data_Start_Offset+0x30; // Text addon after standard header.

	if (EvaluateCommandlineParams(argc, argv, &iTapeBufferSize, filename) == -1)
	{
		usage();
		goto exit;
	}

	// Allocate memory for tape image.
	if (AllocateImageBuffer(&pucTapeBuffer, iTapeBufferSize) == -1)
		goto exit;

	// Check if specified image file is already existing.
	if (CAP_isFilePresent(filename) == CAP_Status_OK)
	{
		printf("\nOverwrite existing file? (y/N)");
		if (getchar() != 'y')
			goto exit;
	}

	printf("\n");

	// Create specified image file for writing.
	FuncRes = CAP_CreateFile(&hCAP, filename);
	if (FuncRes != CAP_Status_OK)
	{
		CAP_OutputError(FuncRes);
		goto exit;
	}

	EnterCriticalSection(&CritSec_fd); // Acquire handle flag access.

	if (cbm_driver_open_ex(&fd, NULL) != 0)
	{
		printf("Driver error.\n");
		CAP_CloseFile(&hCAP);
		LeaveCriticalSection(&CritSec_fd);
		goto exit;
	}

	fd_Initialized = TRUE;
	LeaveCriticalSection(&CritSec_fd); // Release handle flag access.

	RetVal = CaptureTape(fd, pucTapeBuffer, iTapeBufferSize, &iCaptureLen);

	EnterCriticalSection(&CritSec_fd); // Acquire handle flag access.
	cbm_driver_close(fd);
	fd_Initialized = FALSE;
	LeaveCriticalSection(&CritSec_fd); // Release handle flag access.

	if (RetVal != 0)
	{
		CAP_CloseFile(&hCAP);
		goto exit;
	}

	// Write tape image to specified image file.
	RetVal = WriteTapeBufferToCaptureFile(hCAP, pucTapeBuffer, iCaptureLen);

	FuncRes = CAP_CloseFile(&hCAP);
	if (FuncRes != CAP_Status_OK)
	{
		CAP_OutputError(FuncRes);
		goto exit;
	}

	printf("Capture file successfully created.\n");

	exit:
	DeleteCriticalSection(&CritSec_fd);
	DeleteCriticalSection(&CritSec_BreakHandler);
   	if (pucTapeBuffer != NULL) free(pucTapeBuffer);
   	printf("\n");
   	return RetVal;
}
