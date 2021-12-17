/*
 *  CBM 1530/1531 tape routines.
 *  Copyright 2012 Arnd Menge, arnd(at)jonnz(dot)de
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

// Start/stop delay
BOOL             StartDelayActivated = FALSE,
                 StopDelayActivated = FALSE;
unsigned __int32 StartDelay = 0, // Write start delay (replaces first timestamp)
                 StopDelay = 0;  // Motor stop delay after last signal edge was written


void usage(void)
{
	printf("Usage: tapwrite [-aX] [-bY] [-bz] <filename.cap>\n");
	printf("\n");
	printf("  -aX: wait X seconds before writing first signal (optional)\n");
	printf("  -bY: keep on record Y seconds after last signal (optional)\n");
	printf("  -bz: keep on record max time after last signal (optional)\n");
	printf("\n");
	printf("Examples:\n");
	printf("  tapwrite myfile.cap\n");
	printf("  tapwrite -a15 myfile.cap\n");
	printf("  tapwrite -a15 -b30 myfile.cap\n");
	printf("  tapwrite -a15 -bz myfile.cap\n");
}


__int32 EvaluateCommandlineParams(__int32 argc, __int8 *argv[], __int8 filename[_MAX_PATH])
{
	unsigned __int8 bStartDelay = 0, bStopDelay = 0, bMaxStopDelay = 0;

	if ((argc < 2) || (4 < argc))
	{
		printf("Error: invalid number of commandline parameters.\n\n");
		return -1;
	}

	// Evaluate flags.
	while (--argc && (*(++argv)[0] == '-'))
	{
		if ((*argv)[1] == 'a')
		{
			StartDelayActivated = TRUE;
			StartDelay = atoi(&(argv[0][2]));
			bStartDelay++;
		}
		else if (strcmp(*argv,"-bz") == 0)
		{
			StopDelayActivated = TRUE;
			StopDelay = 0xffffffff; // max seconds
			bMaxStopDelay++;
		}
		else if ((*argv)[1] == 'b')
		{
			StopDelay = atoi(&(argv[0][2]));
			if (StopDelay > 0) StopDelayActivated = TRUE;
			bStopDelay++;
		}
		else
		{
			printf("\nError: invalid commandline parameter.\n\n");
			return -1;
		}
	}

	if (bStartDelay == 0)
	{
		printf("* Start delay: not activated\n"); // no start delay
	}
	else if (bStartDelay == 1)
	{
		if (StartDelay == 0)
			printf("* Start delay: minimum / 100us\n", StartDelay);
		else if (StartDelay == 1)
			printf("* Start delay: %u second\n", StartDelay);
		else
			printf("* Start delay: %u seconds\n", StartDelay);
	}
	else if (bStartDelay > 1)
	{
		printf("\nError: Start delay specified more than once.\n\n");
		return -1;
	}

	if ((bMaxStopDelay+bStopDelay) > 1)
	{
		printf("\nError: Stop delay specified more than once.\n\n");
		return -1;
	}
	else if ((bMaxStopDelay+bStopDelay) == 0)
	{
		printf("* Stop delay: not activated\n"); // no stop delay
	}
	else if (bMaxStopDelay == 1)
	{
		printf("* Stop delay: maximum (19 hours) / until <STOP>\n");
	}
	else if (bStopDelay == 1)
	{
		if (StopDelay == 0)
		{
			printf("* Stop delay: not activated\n"); // no stop delay
		}
		else if (StopDelay == 1)
			printf("* Stop delay: %u second\n", StopDelay);
		else
			printf("* Stop delay: %u seconds\n", StopDelay);
	}

	if (strlen(argv[0]) >= _MAX_PATH)
	{
		printf("\nError: Filename too long.\n\n");
		return -1;
	}

	strcpy(filename, argv[0]);

	printf("\n");

	return 0;
}


__int32 AllocateImageBuffer(HANDLE hCAP, void **ppucTapeBuffer, __int32 *piTapeBufferSize)
{
	__int32 FuncRes;

	// Return file size of image file (moves file pointer).
	FuncRes = CAP_GetFileSize(hCAP, piTapeBufferSize);
	if (FuncRes != CAP_Status_OK)
	{
		CAP_OutputError(FuncRes);
		return -1;
	}

	if (*piTapeBufferSize < 0)
	{
		printf("Error: Illegal tape image buffer size.\n");
		return -1;
	}

	// Allocate memory for tape image.
	*ppucTapeBuffer = malloc(*piTapeBufferSize);
	if (*ppucTapeBuffer == NULL)
	{
		printf("Error: Could not allocate memory for capture data.\n");
		return -1;
	}

	// Initialize memory for tape image.
	memset(*ppucTapeBuffer, 0x00, *piTapeBufferSize);

	return 0;
}


// Print tape length to console.
void OutputTapeLength(unsigned __int32 uiTotalTapeTimeSeconds)
{
	unsigned __int32 hours, mins, secs;

	hours = (uiTotalTapeTimeSeconds/3600);
	printf("Tape recording time: %uh", hours);
	mins = ((uiTotalTapeTimeSeconds - hours*3600)/60);
	printf(" %um", mins);
	secs = ((uiTotalTapeTimeSeconds - hours*3600) - mins*60);
	printf(" %us\n\n", secs);
}


// Read tape image into memory.
__int32 ReadCaptureFile(HANDLE hCAP, unsigned __int8 *pucTapeBuffer, __int32 *piCaptureLen)
{
	unsigned __int64 ui64Delta = 0, ShortWarning, ShortError, MinLength, ui64TotalTapeTime = 0;
	unsigned __int32 uiTotalTapeTimeSeconds;
	__int32          FuncRes;
	BOOL             FirstSignal = TRUE;

	// Seek to start of image file and read image header, extract & verify header contents, seek to start of image data.
	FuncRes = CAP_ReadHeader(hCAP);
	if (FuncRes != CAP_Status_OK)
	{
		CAP_OutputError(FuncRes);
		return -1;
	}

	// Get all header entries at once.
	FuncRes = CAP_GetHeader(hCAP, &CAP_Precision, &CAP_Machine, &CAP_Video, &CAP_StartEdge, &CAP_SignalFormat, &CAP_SignalWidth, &CAP_StartOfs);
	if (FuncRes != CAP_Status_OK)
	{
		CAP_OutputError(FuncRes);
		return -1;
	}

	if (CAP_Precision == 16)
	{
		ShortWarning = 16*75; // 75us
		ShortError = 16*60;   // 60us
	}
	else
	{
		ShortWarning = 75; // 75us
		ShortError = 60;   // 60us
	}

	// Keep space for leading number of deltas.
	*piCaptureLen = 5;

	// Read timestamps, convert to 16MHz hardware resolution if necessary.
	while ((FuncRes = CAP_ReadSignal(hCAP, &ui64Delta, NULL)) == CAP_Status_OK)
	{
		if (FirstSignal)
		{
			// Replace first timestamp with start delay if requested
			if (StartDelayActivated == TRUE)
			{
				if (StartDelay == 0)
					ui64Delta = 1600; // 100us minimum
				else
				{
					ui64Delta = StartDelay;
					ui64Delta *= 15625; //16000000;
					ui64Delta <<= 10;
				}
			}
			FirstSignal = FALSE;
		}
		else
			if (CAP_Precision == 1) ui64Delta <<= 4; // Convert from 1MHz to 16MHz.

		if (ui64Delta < ShortWarning) printf("Warning - Short signal length detected: 0x%.10X\n", ui64Delta);
		if (ui64Delta < ShortError)
		{
			printf("Warning - Replaced by minimum signal length.\n");
			ui64Delta = ShortError;
		}

		ui64TotalTapeTime += ui64Delta;

		if (ui64Delta < 0x8000)
		{
			// Short signal (<2ms)
			(*piCaptureLen) += 2;
		}
		else
		{
			// Long signal (>=2ms)
			(*piCaptureLen) += 5;
			pucTapeBuffer[*piCaptureLen-5] = (unsigned __int8) (((ui64Delta >> 32) & 0x7f) | 0x80); // MSB must be 1.
			pucTapeBuffer[*piCaptureLen-4] = (unsigned __int8)  ((ui64Delta >> 24) & 0xff);
			pucTapeBuffer[*piCaptureLen-3] = (unsigned __int8)  ((ui64Delta >> 16) & 0xff);
		}
		pucTapeBuffer[*piCaptureLen-2] = (unsigned __int8) ((ui64Delta >>  8) & 0xff);
		pucTapeBuffer[*piCaptureLen-1] = (unsigned __int8) (ui64Delta & 0xff);
	}

	if (FuncRes == CAP_Status_Error_Reading_data)
	{
		CAP_OutputError(FuncRes);
		return -1;
	}

	// Add final timestamp for stop delay
	if (StopDelayActivated == TRUE)
	{
		if (StopDelay == 0xffffffff)
			ui64Delta = 0xffffffffff;
		else
		{
			ui64Delta = StopDelay;
			ui64Delta *= 15625;
			ui64Delta <<= 10; //16000000;
		}

		ui64TotalTapeTime += ui64Delta;

		if (ui64Delta < 0x8000)
		{
			// Short signal (<2ms)
			(*piCaptureLen) += 2;
		}
		else
		{
			// Long signal (>=2ms)
			(*piCaptureLen) += 5;
			pucTapeBuffer[*piCaptureLen-5] = (unsigned __int8) (((ui64Delta >> 32) & 0x7f) | 0x80); // MSB must be 1.
			pucTapeBuffer[*piCaptureLen-4] = (unsigned __int8)  ((ui64Delta >> 24) & 0xff);
			pucTapeBuffer[*piCaptureLen-3] = (unsigned __int8)  ((ui64Delta >> 16) & 0xff);
		}
		pucTapeBuffer[*piCaptureLen-2] = (unsigned __int8) ((ui64Delta >>  8) & 0xff);
		pucTapeBuffer[*piCaptureLen-1] = (unsigned __int8) (ui64Delta & 0xff);
	}

	// Send number of delta bytes first.
	pucTapeBuffer[0] = 0x80;
	pucTapeBuffer[1] = ((*piCaptureLen-5) >> 24) & 0xff;
	pucTapeBuffer[2] = ((*piCaptureLen-5) >> 16) & 0xff;
	pucTapeBuffer[3] = ((*piCaptureLen-5) >>  8) & 0xff;
	pucTapeBuffer[4] =  (*piCaptureLen-5) & 0xff;

	// Calculate tape recording length.
	uiTotalTapeTimeSeconds = (unsigned __int32) ((ui64TotalTapeTime >> 10)/15625); //16000000;
	OutputTapeLength(uiTotalTapeTimeSeconds);

	return 0;
}


__int32 WriteTape(CBM_FILE fd, unsigned __int8 *pucTapeBuffer, unsigned __int32 uiCaptureLen)
{
	__int32         Status, BytesRead, BytesWritten, FuncRes;
	unsigned __int8 WriteConfig, WriteConfig2;

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

	// Prepare tape write configuration.
	// Write signal is inverted read signal.
	if (CAP_StartEdge == CAP_StartEdge_Falling)
		WriteConfig = ~(unsigned __int8)XUM1541_TAP_WRITE_STARTFALLEDGE; // Start writing with rising edge.
	else
		WriteConfig = (unsigned __int8)XUM1541_TAP_WRITE_STARTFALLEDGE; // Start writing with falling edge.

	// Check abort flag.
	if (AbortTapeOps)
		return -1;

	// Upload tape write configuration.
	//   Status values:
	//   - Tape_Status_OK_Config_Uploaded
	//   - Tape_Status_ERROR_usbRecvByte
	//   FuncRes values concerning tape mode:
	//   - XUM1541_Error_NoTapeSupport
	//   - XUM1541_Error_NoDiskTapeMode
	//   - XUM1541_Error_TapeCmdInDiskMode
	FuncRes = cbm_tap_upload_config(fd, &WriteConfig, 1, &Status, &BytesWritten);
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

	// Download tape write configuration and check.
	//   Status values:
	//   - Tape_Status_OK_Config_Downloaded
	//   - Tape_Status_ERROR_usbSendByte
	//   FuncRes values concerning tape mode:
	//   - XUM1541_Error_NoTapeSupport
	//   - XUM1541_Error_NoDiskTapeMode
	//   - XUM1541_Error_TapeCmdInDiskMode
	FuncRes = cbm_tap_download_config(fd, &WriteConfig2, 1, &Status, &BytesRead);
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
		printf("\nReturned error [download_config]: Invalid data size (%d).\n", BytesRead);
		return -1;
	}
	if ((WriteConfig & 0x60) != (WriteConfig2 & 0x60))
	{
		printf("\nError [download_config]: Configuration mismatch.\n");
		return -1;
	}

	// Check abort flag.
	if (AbortTapeOps)
		return -1;

	// Prepare tape firmware for write.
	//   Status values:
	//   - Tape_Status_OK_Device_Configured_for_Write
	//   - Tape_Status_ERROR_Device_Disconnected
	//   - Tape_Status_ERROR_Not_In_Tape_Mode
	//   FuncRes values concerning tape mode:
	//   - XUM1541_Error_NoTapeSupport
	//   - XUM1541_Error_NoDiskTapeMode
	//   - XUM1541_Error_TapeCmdInDiskMode
	FuncRes = cbm_tap_prepare_write(fd, &Status);
	if (FuncRes < 0)
	{
		printf("\nReturned error [prepare_write]: ");
		if (OutputFuncError(FuncRes) < 0)
			printf("%d\n", FuncRes);
		return -1;
	}
	if (Status != Tape_Status_OK_Device_Configured_for_Write)
	{
		printf("\nReturned error [prepare_write]: ");
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

	printf("Press <RECORD> and <PLAY> on tape.\n");

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

	printf("\nWriting tape...\n");

	//   Status values:
	//   - Tape_Status_OK_Write_Finished
	//   - Tape_Status_ERROR_Write_Interrupted_By_Stop
	//   - Tape_Status_ERROR_Sense_Not_On_Record
	//   - Tape_Status_ERROR_Device_Not_Configured
	//   - Tape_Status_ERROR_Device_Disconnected
	//   FuncRes values concerning tape mode:
	//   - XUM1541_Error_NoTapeSupport
	//   - XUM1541_Error_NoDiskTapeMode
	//   - XUM1541_Error_TapeCmdInDiskMode
	FuncRes = cbm_tap_start_write(fd, pucTapeBuffer, uiCaptureLen, &Status, &BytesWritten);
	if (FuncRes < 0)
	{
		printf("\nReturned error [write]: ");
		if (OutputFuncError(FuncRes) < 0)
			printf("%d\n", FuncRes);
		return -1;
	}
	if (Status != Tape_Status_OK_Write_Finished)
	{
		printf("\nReturned error [write]: ");
		if (OutputError(Status) < 0)
			printf("%d\n", Status);
		return -1;
	}
	if (uiCaptureLen != BytesWritten)
	{
		printf("\nError [write]: Short write.\n");
		return -1;
	}

	// Check abort flag.
	if (AbortTapeOps)
		return -1;

	printf("\nWriting finished OK.\n");
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
//    0: tape writing finished ok
//   -1: an error occurred
int ARCH_MAINDECL main(int argc, char *argv[])
{
	HANDLE          hCAP;
	unsigned __int8 *pucTapeBuffer = NULL;
	__int8          filename[_MAX_PATH];
	__int32         iCaptureLen = 0, iTapeBufferSize;
	__int32         FuncRes, RetVal = -1;

	printf("\ntapwrite v1.00 - Commodore 1530/1531 tape mastering software\n");
	printf("Copyright 2012 Arnd Menge\n\n");

	InitializeCriticalSection(&CritSec_fd);
	InitializeCriticalSection(&CritSec_BreakHandler);

	if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE) BreakHandler, TRUE))
		printf("Ctrl-C break handler not installed.\n\n");

	if (EvaluateCommandlineParams(argc, argv, filename) == -1)
	{
		usage();
		goto exit;
	}

	// Open specified image file for reading.
	FuncRes = CAP_OpenFile(&hCAP, filename);
	if (FuncRes != CAP_Status_OK)
	{
		CAP_OutputError(FuncRes);
		goto exit;
	}

	// Allocate memory for tape image.
	if (AllocateImageBuffer(hCAP, &pucTapeBuffer, &iTapeBufferSize) == -1)
	{
		CAP_CloseFile(&hCAP);
		goto exit;
	}

	// Read tape image into memory.
	RetVal = ReadCaptureFile(hCAP, pucTapeBuffer, &iCaptureLen);

	CAP_CloseFile(&hCAP);

	if (RetVal == -1)
		goto exit;

	EnterCriticalSection(&CritSec_fd); // Acquire handle flag access.

	if (cbm_driver_open_ex(&fd, NULL) != 0)
	{
		printf("Driver error.\n");
		LeaveCriticalSection(&CritSec_fd);
		goto exit;
	}

	fd_Initialized = TRUE;
	LeaveCriticalSection(&CritSec_fd); // Release handle flag access.

	RetVal = WriteTape(fd, pucTapeBuffer, iCaptureLen);

	EnterCriticalSection(&CritSec_fd); // Acquire handle flag access.
	cbm_driver_close(fd);
	fd_Initialized = FALSE;
	LeaveCriticalSection(&CritSec_fd); // Release handle flag access.

    exit:
	DeleteCriticalSection(&CritSec_fd);
	DeleteCriticalSection(&CritSec_BreakHandler);
   	if (pucTapeBuffer != NULL) free(pucTapeBuffer);
   	printf("\n");
   	return RetVal;
}
