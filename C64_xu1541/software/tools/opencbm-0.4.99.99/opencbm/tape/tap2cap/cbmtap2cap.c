/*
 *  CBM 1530/1531 tape routines.
 *  Copyright 2012 Arnd Menge, arnd(at)jonnz(dot)de
*/

#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

#include <arch.h>
#include "cap.h"
#include "tap-cbm.h"
#include "misc.h"

#define FREQ_C64_PAL    985248
#define FREQ_C64_NTSC  1022727
#define FREQ_VIC_PAL   1108405
#define FREQ_VIC_NTSC  1022727
#define FREQ_C16_PAL    886724
#define FREQ_C16_NTSC   894886

#define NeedSplit            2
#define NoSplit              3

// Global variables
unsigned __int8   CAP_Machine, CAP_Video, CAP_StartEdge, CAP_SignalFormat;
unsigned __int32  CAP_Precision, CAP_SignalWidth, CAP_StartOfs;
unsigned __int8   TAP_Machine, TAP_Video, TAPv;
unsigned __int32  TAP_ByteCount;


__int32 HandleDeltaAndWriteToCAP(HANDLE hCAP, unsigned __int64 ui64Delta, unsigned __int8 uiSplit)
{
	unsigned __int64 ui64SplitLen;
	__int32          FuncRes;

	if (uiSplit == NeedSplit)
	{
		// Write two halfwaves.
		ui64SplitLen = ui64Delta/2;
		Check_CAP_Error_TextRetM1(CAP_WriteSignal(hCAP, ui64SplitLen, NULL));
		ui64SplitLen = ui64Delta-ui64SplitLen;
		Check_CAP_Error_TextRetM1(CAP_WriteSignal(hCAP, ui64SplitLen, NULL));
	}
	else
	{
		// Write one halfwave.
		Check_CAP_Error_TextRetM1(CAP_WriteSignal(hCAP, ui64Delta, NULL));
	}

	return 0;
}


__int32 Initialize_CAP_header_and_return_frequency(HANDLE hCAP, HANDLE hTAP, unsigned __int32 *puiFreq)
{
	__int32 FuncRes;

	// Determine target machine & video.

	if (TAP_Machine == TAP_Machine_C64)
	{
		printf("* C64\n");
		CAP_Machine = CAP_Machine_C64;
	}
	else if (TAP_Machine == TAP_Machine_C16)
	{
		printf("* C16\n");
		CAP_Machine = CAP_Machine_C16;
	}
	else if (TAP_Machine == TAP_Machine_VC20)
	{
		printf("* VC20\n");
		CAP_Machine = CAP_Machine_VC20;
	}
	else return -1;

	if (TAP_Video == TAP_Video_PAL)
	{
		printf("* PAL\n");
		CAP_Video = CAP_Video_PAL;
	}
	else if (TAP_Video == TAP_Video_NTSC)
	{
		printf("* NTSC\n");
		CAP_Video = CAP_Video_NTSC;
	}
	else return -1;

	// Initialize & write CAP image header.

	CAP_Precision    = 1;                         // Default: 1us signal precision.
	CAP_StartEdge    = CAP_StartEdge_Falling;     // Default: Start with falling signal edge.
	CAP_SignalFormat = CAP_SignalFormat_Relative; // Default: Relative timings instead of absolute.
	CAP_SignalWidth  = CAP_SignalWidth_40bit;     // Default: 40bit.
	CAP_StartOfs     = CAP_Default_Data_Start_Offset+0x30; // Text addon after standard header.

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

	FuncRes = CAP_WriteHeaderAddon(hCAP, "   Created by       TAP2CAP     ----------------", 0x30);
	if (FuncRes != CAP_Status_OK)
	{
		CAP_OutputError(FuncRes);
		return -1;
	}

	// Determine frequency.

	if (     (TAP_Machine == TAP_Machine_C64)  && (TAP_Video == TAP_Video_PAL))
		*puiFreq = FREQ_C64_PAL;
	else if ((TAP_Machine == TAP_Machine_C64)  && (TAP_Video == TAP_Video_NTSC))
		*puiFreq = FREQ_C64_NTSC;
	else if ((TAP_Machine == TAP_Machine_VC20) && (TAP_Video == TAP_Video_PAL))
		*puiFreq = FREQ_VIC_PAL;
	else if ((TAP_Machine == TAP_Machine_VC20) && (TAP_Video == TAP_Video_NTSC))
		*puiFreq = FREQ_VIC_NTSC;
	else if ((TAP_Machine == TAP_Machine_C16)  && (TAP_Video == TAP_Video_PAL))
		*puiFreq = FREQ_C16_PAL;
	else if ((TAP_Machine == TAP_Machine_C16)  && (TAP_Video == TAP_Video_NTSC))
		*puiFreq = FREQ_C16_NTSC;
	else
	{
		printf("Error: Can't determine machine frequency.\n");
		return -1;
	}

	printf("\n");

	return 0;
}


// Convert CBM TAP to CAP format.
__int32 CBMTAP2CAP(HANDLE hCAP, HANDLE hTAP)
{
	unsigned __int64 ui64Delta;
	unsigned __int32 uiDelta, uiFreq;
	unsigned __int32 TAP_Counter = 0; // CAP & TAP file byte counters.
	unsigned __int8  ch;
	__int32          FuncRes;

	// Seek to & read image header, extract & verify header contents.
	FuncRes = TAP_CBM_ReadHeader(hTAP);
	if (FuncRes != TAP_CBM_Status_OK)
	{
		TAP_CBM_OutputError(FuncRes);
		return -1;
	}

	// Get all header entries at once.
	FuncRes = TAP_CBM_GetHeader(hTAP, &TAP_Machine, &TAP_Video, &TAPv, &TAP_ByteCount);
	if (FuncRes != TAP_CBM_Status_OK)
	{
		TAP_CBM_OutputError(FuncRes);
		return -1;
	}

	if (Initialize_CAP_header_and_return_frequency(hCAP, hTAP, &uiFreq) != 0)
		return -1;

	// Start conversion TAP->CAP.

	// Start with 100us delay (can be replaced with specified start delay in tapwrite).
	ui64Delta = CAP_Precision*100;
	if (HandleDeltaAndWriteToCAP(hCAP, ui64Delta, NoSplit) == -1)
		return -1;

	// Conversion loop.
	while ((FuncRes = TAP_CBM_ReadSignal(hTAP, &uiDelta, &TAP_Counter)) == TAP_CBM_Status_OK)
	{
		ui64Delta = uiDelta;
		ui64Delta = (ui64Delta*1000000*CAP_Precision+uiFreq/2)/uiFreq;

		if ((TAPv == TAPv0) || (TAPv == TAPv1))
		{
			// Generate two halfwaves.
			if (HandleDeltaAndWriteToCAP(hCAP, ui64Delta, NeedSplit) == -1)
				return -1;
		}
		else
		{
			// Generate one halfwave.
			if (HandleDeltaAndWriteToCAP(hCAP, ui64Delta, NoSplit) == -1)
				return -1;
		}
	}

	if (FuncRes == TAP_CBM_Status_Error_Reading_data)
	{
		TAP_CBM_OutputError(FuncRes);
		return -1;
	}

	return 0;
}
