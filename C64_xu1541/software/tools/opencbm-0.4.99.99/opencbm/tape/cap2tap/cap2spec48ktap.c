/*
 *  CBM 1530/1531 tape routines.
 *  Copyright 2012 Arnd Menge, arnd(at)jonnz(dot)de
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <Windows.h>

#include "cap.h"

// Define pulses.
#define ShortPulse 1
#define LongPulse  2
#define PausePulse 3

// Define waveforms.
#define HalfWave   0
#define ShortWave  1
#define LongWave   2
#define PauseWave  3
#define ErrorWave  4


// Convert CAP to Spectrum48K TAP format. *EXPERIMENTAL*
__int32 CAP2SPEC48KTAP(HANDLE hCAP, FILE *TapFile)
{
	unsigned __int8  DBGFLAG = 0; // 1 = Debug output
	unsigned __int8  *zb; // Spectrum48K TAP image buffer.
	unsigned __int8  ch = 0;
	unsigned __int64 ui64Delta, ui64Len;
	unsigned __int32 Timer_Precision_MHz;
	__int32          FuncRes;    // Function call result.
	__int32          RetVal = 0; // Default return value.

	// Declare variables holding current/last pulse & wave information.
	unsigned __int8 LastPulse = PausePulse;
	unsigned __int8 Pulse     = PausePulse;
	unsigned __int8 Wave      = HalfWave;

	// Declare image pointers.
	unsigned __int32 BlockStart = 0; // Data block start in image.
	unsigned __int32 BlockPos   = 2; // Current position in image.

	// Declare bit & byte counters.
	unsigned __int8  BitCount         = 0; // Data block bit counter.
	unsigned __int32 ByteCount        = 0; // Data block data byte counter.
	unsigned __int32 DataPulseCounter = 0; // Data block pulse counter.
	unsigned __int32 BlockByteCounter = 0; // Data block byte counter.

	// Get memory for Spectrum48K TAP image buffer. Should be dynamic size.
	zb = (unsigned __int8 *)malloc((size_t)10000000);
	if (zb == NULL)
	{
		printf("Error: Not enough memory for Spectrum48K TAP image buffer.");
		return -1;
	}
	memset((void *)zb, (__int32)0, (size_t)10000000);

	// Seek to start of image file and read image header, extract & verify header contents, seek to start of image data.
	FuncRes = CAP_ReadHeader(hCAP);
	if (FuncRes != CAP_Status_OK)
	{
		CAP_OutputError(FuncRes);
		RetVal = -1;
		goto exit;
	}

	// Return timestamp precision from header.
	FuncRes = CAP_GetHeader_Precision(hCAP, &Timer_Precision_MHz);
	if (FuncRes != CAP_Status_OK)
	{
		CAP_OutputError(FuncRes);
		RetVal = -1;
		goto exit;
	}

	// Skip first halfwave (time until first pulse occurs).
	FuncRes = CAP_ReadSignal(hCAP, &ui64Delta, NULL);
	if (FuncRes == CAP_Status_OK_End_of_file)
	{
		printf("Error: Empty image file.");
		RetVal = -1;
		goto exit;
	}
	else if (FuncRes == CAP_Status_Error_Reading_data)
	{
		CAP_OutputError(FuncRes);
		RetVal = -1;
		goto exit;
	}

	// While CAP 5-byte timestamp available.
	while ((FuncRes = CAP_ReadSignal(hCAP, &ui64Delta, NULL)) == CAP_Status_OK)
	{
		ui64Len = (ui64Delta+(Timer_Precision_MHz/2))/Timer_Precision_MHz;

		if (DBGFLAG == 1) printf("%I64u ", ui64Len);
		
		LastPulse = Pulse;

		// Evaluate current pulse width.
		if ((150 <= ui64Len) && (ui64Len <= 360))
		{
			Pulse = ShortPulse;
			if (DBGFLAG == 1) printf("(SP) ");
		}
		else if ((360 < ui64Len) && (ui64Len < 550))
		{
			Pulse = LongPulse;
			if (DBGFLAG == 1) printf("(LP) ");
		}
		else // <150 or >550
		{
			Pulse = PausePulse;
			if (DBGFLAG == 1) printf("(PP) ");
		}


		if (Pulse == PausePulse)
		{
			DataPulseCounter = 0;
			BlockByteCounter = 0;

			if (ByteCount > 0)
			{
				// Calculate block size and write to TAP image.
				zb[BlockStart  ] = ByteCount & 0xff;
				zb[BlockStart+1] = (ByteCount >> 8) & 0xff;
				if (DBGFLAG == 1) printf("Block size = %u", ByteCount);
				BlockStart = BlockPos;
				BlockPos += 2;
			}
			ByteCount = 0;
			BitCount = 0;

		}
		else DataPulseCounter++;


		// Evaluate waveform after every second data pulse.
		if ((DataPulseCounter > 0) && ((DataPulseCounter % 2) == 0))
		{

			if ((LastPulse == ShortPulse) && (Pulse == ShortPulse))
			{
				Wave = ShortWave;
				if (DBGFLAG == 1) printf("(SW) ");
			}
			else if ((LastPulse == LongPulse) && (Pulse == LongPulse))
			{
				Wave = LongWave;
				if (DBGFLAG == 1) printf("(LW) ");
			}
			else
			{
				Wave = ErrorWave;
				if (DBGFLAG == 1) printf("(EW) ");
			}

			if ((Wave == ShortWave) || (Wave == LongWave))
				BlockByteCounter++;
			else
				BlockByteCounter = 0;


			if (BlockByteCounter > 1)
			{
				// We found a bit.
				BitCount++;

				// Evaluate wave.
				if (Wave == ShortWave)
				{
					ch = (ch << 1);
					if (DBGFLAG == 1) printf("(0)");
				}
				else if (Wave == LongWave)
				{
					ch = (ch << 1) + 1;
					if (DBGFLAG == 1) printf("(1)");
				}

				if (BitCount == 8)
				{
					ByteCount++; // Increase byte counter.
					BitCount = 0; // Reset bit counter.

					zb[BlockPos++] = ch; // Store byte to image.
					if (DBGFLAG == 1) printf(" -----> 0x%.2x <%c>", ch, ch);

					if (ByteCount == 1)
					{
						// Evaluate first block byte.
						if (DBGFLAG == 1)
						{
							if (ch == 0)
								printf(" [Header]");
							else if (ch == 0xff)
								printf(" [Data]");
							else
								printf(" [Unknown block!]");
						}
					}
				} // if (BitCount == 8)

			} // if (BlockCounter > 1)
			else if (DBGFLAG == 1) printf("(x)");
		} // if ((DataPulseCounter > 0) && ((DataPulseCounter % 2) == 0))
	} // While CAP 5-byte timestamp available.

	if (FuncRes == CAP_Status_Error_Reading_data)
	{
		CAP_OutputError(FuncRes);
		RetVal = -1;
		goto exit;
	}

	// Handle final data block, if exists.
	if (ByteCount > 0)
	{
		// Calculate block size and write to TAP image.
		zb[BlockStart  ] = ByteCount & 0xff;
		zb[BlockStart+1] = (ByteCount >> 8) & 0xff;
		if (DBGFLAG == 1) printf("Block size = %u\n", ByteCount);
		BlockStart = BlockPos;
		BlockPos += 2;
	}

	if (BlockPos > 2)
		fwrite((const void *)zb, (size_t)(BlockPos-2), (size_t)1, (FILE *)TapFile);
	else
	{
		printf("Error: Empty image file.\n");
		RetVal = -1;
		goto exit;
	}

exit:
	// Release memory for Spectrum48K TAP image buffer.
	free((void *)zb);

	return RetVal;
}
