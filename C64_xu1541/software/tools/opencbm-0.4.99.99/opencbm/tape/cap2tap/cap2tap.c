/*
 *  CBM 1530/1531 tape routines.
 *  Copyright 2012 Arnd Menge, arnd(at)jonnz(dot)de
*/

#include <stdio.h>
#include <stdlib.h>

#include <arch.h>
#include "cap2cbmtap.h"
#include "cap2spec48ktap.h"
#include "cap.h"
#include "tap-cbm.h"
#include "misc.h"


void usage(void)
{
	printf("\nUsage:   cap2tap <input.cap> <output.tap>\n\n");
	printf("Example: cap2tap myfile.cap myfile.tap\n");
}


__int32 Evaluate_Commandline_Params(__int32 argc, __int8 *argv[])
{
	if (argc == 3) return 0;
	else return -1;
}


// Main routine.
//   Return values:
//    0: conversion finished ok
//   -1: an error occurred
int ARCH_MAINDECL main(int argc, char *argv[])
{
	HANDLE          hCAP, hTAP;
	FILE            *fd; // Experimental Spectrum48K support.
	unsigned __int8 CAP_Machine;
	__int32         FuncRes, RetVal = -1;

	printf("\nCAP2TAP v1.00 - ZoomTape CAP image to TAP image conversion\n");
	printf("Copyright 2012 Arnd Menge\n\n");

	if (Evaluate_Commandline_Params(argc, argv) == -1)
	{
		usage();
		goto exit;
	}

	// Open specified image file for reading.
	FuncRes = CAP_OpenFile(&hCAP, argv[1]);
	if (FuncRes != CAP_Status_OK)
	{
		CAP_OutputError(FuncRes);
		goto exit;
	}

	// Seek to start of image file and read image header, extract & verify header contents, seek to start of image data.
	FuncRes = CAP_ReadHeader(hCAP);
	if (FuncRes != CAP_Status_OK)
	{
		CAP_OutputError(FuncRes);
		CAP_CloseFile(&hCAP);
		goto exit;
	}

	// Get target machine type from header.
	FuncRes = CAP_GetHeader_Machine(hCAP, &CAP_Machine);
	if (FuncRes != CAP_Status_OK)
	{
		CAP_OutputError(FuncRes);
		CAP_CloseFile(&hCAP);
		goto exit;
	}

	// Check if specified TAP image file is already existing.
	if (TAP_CBM_isFilePresent(argv[2]) == TAP_CBM_Status_OK)
	{
		printf("Overwrite existing file? (y/N)");
		if (getchar() != 'y')
		{
			CAP_CloseFile(&hCAP);
			goto exit;
		}
		printf("\n");
	}

	// Create specified TAP image file for writing.
	if (CAP_Machine == CAP_Machine_Spec48K)
	{
		// Spectrum48K support is *EXPERIMENTAL*
		fd = fopen(argv[2], "wb");
		if (fd == NULL)
		{
			printf("Error creating TAP file.");
			CAP_CloseFile(&hCAP);
			goto exit;
		}
	}
	else
	{
		FuncRes = TAP_CBM_CreateFile(&hTAP, argv[2]);
		if (FuncRes != CAP_Status_OK)
		{
			CAP_OutputError(FuncRes);
			CAP_CloseFile(&hCAP);
			goto exit;
		}
	}

	printf("Converting: %s -> %s\n\n", argv[1], argv[2]);

	if (CAP_Machine == CAP_Machine_Spec48K)
	{
		// Convert CAP to Spectrum48K TAP format. *EXPERIMENTAL*
		RetVal = CAP2SPEC48KTAP(hCAP, fd);

		CAP_CloseFile(&hCAP);

		if (fclose(fd) != 0)
		{
			printf("Error: Closing TAP file failed.");
			RetVal = -1;
		}
	}
	else
	{
		// Convert CAP to CBM TAP format.
		RetVal = CAP2CBMTAP(hCAP, hTAP);

		CAP_CloseFile(&hCAP);

		FuncRes = TAP_CBM_CloseFile(&hTAP);
		if (FuncRes != TAP_CBM_Status_OK)
		{
			TAP_CBM_OutputError(FuncRes);
			RetVal = -1;
		}
	}

	if (RetVal == 0)
		printf("Conversion successful.");

    exit:
   	printf("\n");
   	return RetVal;
}
