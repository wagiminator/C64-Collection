/*
 *  CBM 1530/1531 tape routines.
 *  Copyright 2012 Arnd Menge, arnd(at)jonnz(dot)de
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include <malloc.h>

#include "cap.h"

#define Default_CAP_Header_Size 0xA0

#define SEEK_START_OF_FILE 1
#define SEEK_START_OF_DATA 2

#define DETAILED_INFO(rv) {fprintf(stderr, "Error : %d\nModule: %s\nBuilt : %s %s\nLine  : %d\n", rv, __FILE__, __DATE__, __TIME__, __LINE__);}

#define ASSERT(x, rv) {if (!x) {DETAILED_INFO(rv); return rv;}}

typedef struct _INFOBLOCK {
	unsigned int  MemTag;
	FILE          *fd;
	char          header[Default_CAP_Header_Size+1]; // + 0-termination
	unsigned char Machine, Video, StartEdge, SignalFormat;
	unsigned int  Precision, SignalWidth, StartOfs;
	unsigned int  MemTag2;
} INFOBLOCK, *PINFOBLOCK;


// Exported function.
// Create (overwrite) an image file for writing.
int CAP_CreateFile(HANDLE *hHandle, char *pcFilename)
{
	PINFOBLOCK pInfoBlock;

	ASSERT(hHandle != 0, CAP_Status_Error_Invalid_Handle);

	pInfoBlock = (struct _INFOBLOCK*)malloc(sizeof(INFOBLOCK));

	ASSERT(pInfoBlock != 0, CAP_Status_Error_Out_of_memory);

	memset(pInfoBlock, 0x00, sizeof(INFOBLOCK));

	// Write memory tags.
	pInfoBlock->MemTag  = 0x5f424943; // CIB_
	pInfoBlock->MemTag2 = 0x4349425f; // _BIC

	pInfoBlock->fd = fopen(pcFilename, "wb");
	if (pInfoBlock->fd == NULL)
	{
		free(pInfoBlock);
		return CAP_Status_Error_Creating_file;
	}

	pInfoBlock->StartOfs = 0;
	*hHandle = (HANDLE) pInfoBlock;

	return CAP_Status_OK;
}


// Exported function.
// Open an existing image file for reading.
int CAP_OpenFile(HANDLE *hHandle, char *pcFilename)
{
	PINFOBLOCK pInfoBlock;

	ASSERT(hHandle != 0, CAP_Status_Error_Invalid_Handle);

	pInfoBlock = (struct _INFOBLOCK*)malloc(sizeof(INFOBLOCK));

	ASSERT(pInfoBlock != 0, CAP_Status_Error_Out_of_memory);

	memset(pInfoBlock, 0x00, sizeof(INFOBLOCK));

	// Write memory tags.
	pInfoBlock->MemTag  = 0x5f424943; // CIB_
	pInfoBlock->MemTag2 = 0x4349425f; // _BIC

	pInfoBlock->fd = fopen(pcFilename, "rb");
	if (pInfoBlock->fd == NULL)
	{
		free(pInfoBlock);
		return CAP_Status_Error_File_not_found;
	}

	pInfoBlock->StartOfs = 0;
	*hHandle = (HANDLE) pInfoBlock;

	return CAP_Status_OK;
}


// Exported function.
// Close an image file.
int CAP_CloseFile(HANDLE *hHandle)
{
	PINFOBLOCK pInfoBlock;

	ASSERT(hHandle != 0, CAP_Status_Error_Invalid_Handle);

	pInfoBlock = (struct _INFOBLOCK*)(*hHandle);

	ASSERT(pInfoBlock != 0, CAP_Status_Error_Invalid_Handle);

	if (pInfoBlock->fd != NULL)
		if (fclose(pInfoBlock->fd) != 0)
			return CAP_Status_Error_Closing_file;

	free(pInfoBlock);

	*hHandle = NULL;

	return CAP_Status_OK;
}


// Exported function.
// Check if a file is already existing.
int CAP_isFilePresent(char *pcFilename)
{
	FILE *fd = fopen(pcFilename, "r");
	if (fd == NULL)
		return CAP_Status_Error_File_not_found;

	fclose(fd);
	return CAP_Status_OK;
}


// Exported function.
// Return file size of image file (moves file pointer).
int CAP_GetFileSize(HANDLE hHandle, int *piFileSize)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, CAP_Status_Error_Invalid_Handle);
	ASSERT(piFileSize != 0, CAP_Status_Error_Invalid_pointer);

	if (fseek(pInfoBlock->fd, 0, SEEK_END) != 0)
		return CAP_Status_Error_Seek_failed;

	*piFileSize = ftell(pInfoBlock->fd);
	rewind(pInfoBlock->fd);

	return CAP_Status_OK;
}


// Internal function.
// Seek to start of image file, or seek start of image data.
int CAP_SeekFile(HANDLE hHandle, char cDestination)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, CAP_Status_Error_Invalid_Handle);
	ASSERT(pInfoBlock->fd != 0, CAP_Status_Error_File_not_open);

	if (cDestination == SEEK_START_OF_FILE)
	{
		if (fseek(pInfoBlock->fd, 0, SEEK_SET) != 0)
			return CAP_Status_Error_Seek_failed;
	}
	else if (cDestination == SEEK_START_OF_DATA)
	{
		if (pInfoBlock->StartOfs < Default_CAP_Header_Size)
			return CAP_Status_Error_Seek_wrong_destination;
		if (fseek(pInfoBlock->fd, pInfoBlock->StartOfs, SEEK_SET) != 0)
			return CAP_Status_Error_Seek_failed;
	}

	return CAP_Status_OK;
}


// Internal function.
// Extract & verify header contents.
int Extract_and_Verify_CAP_Header_Contents(HANDLE hHandle)
{
	unsigned int StartOfs;

	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, CAP_Status_Error_Invalid_Handle);

	// Check CAP file signature.
	if (strcmp(&(pInfoBlock->header[0x00]),"TAPEIMAGE") != 0)
		return CAP_Status_Error_Wrong_signature;

	// Check CAP file version.
	if (strcmp(&(pInfoBlock->header[0x10]),"v1.00") != 0)
		return CAP_Status_Error_Wrong_version;

	// Extract timer precision.
	if      (strcmp(&(pInfoBlock->header[0x20]),"1us") == 0)
		pInfoBlock->Precision = 1;
	else if (strcmp(&(pInfoBlock->header[0x20]),"62.5ns") == 0)
		pInfoBlock->Precision = 16;
	else
		return CAP_Status_Error_Wrong_precision;

	// Extract computer type.
	if      (strcmp(&(pInfoBlock->header[0x30]),"C64") == 0)
		pInfoBlock->Machine = CAP_Machine_C64;
	else if (strcmp(&(pInfoBlock->header[0x30]),"C16") == 0)
		pInfoBlock->Machine = CAP_Machine_C16;
	else if (strcmp(&(pInfoBlock->header[0x30]),"VC20") == 0)
		pInfoBlock->Machine = CAP_Machine_VC20;
	else if (strcmp(&(pInfoBlock->header[0x30]),"Spectrum48K") == 0)
		pInfoBlock->Machine = CAP_Machine_Spec48K;
	else
		return CAP_Status_Error_Wrong_machine;

	// Extract video type.
	if      (strcmp(&(pInfoBlock->header[0x40]),"PAL") == 0)
		pInfoBlock->Video = CAP_Video_PAL;
	else if (strcmp(&(pInfoBlock->header[0x40]),"NTSC") == 0)
		pInfoBlock->Video = CAP_Video_NTSC;
	else if (strcmp(&(pInfoBlock->header[0x40]),"Spectrum48K") == 0)
		pInfoBlock->Video = CAP_Video_Spec48K;
	else
		return CAP_Status_Error_Wrong_video;

	// Check start signal edge.
	if      (strcmp(&(pInfoBlock->header[0x50]),"FallingEdge") == 0)
		pInfoBlock->StartEdge = CAP_StartEdge_Falling;
	else if (strcmp(&(pInfoBlock->header[0x50]),"RisingEdge") == 0)
		pInfoBlock->StartEdge = CAP_StartEdge_Rising;
	else
		return CAP_Status_Error_Wrong_start_signal_edge;

	// Check signal format.
	if (strcmp(&(pInfoBlock->header[0x60]),"Relative") == 0)
		pInfoBlock->SignalFormat = CAP_SignalFormat_Relative;
	else if (strcmp(&(pInfoBlock->header[0x60]),"Absolute") == 0)
		pInfoBlock->SignalFormat = CAP_SignalFormat_Absolute;
	else
		return CAP_Status_Error_Wrong_signal_format;

	// Only compatible with 40bit timestamps.
	if (strcmp(&(pInfoBlock->header[0x70]),"40bit") == 0)
		pInfoBlock->SignalWidth = CAP_SignalWidth_40bit;
	else
		return CAP_Status_Error_Wrong_signal_width;

	// Extract data start offset.
	StartOfs = (unsigned char) (pInfoBlock->header[0x80]);
	StartOfs = (StartOfs << 8) + (unsigned char) (pInfoBlock->header[0x81]);
	StartOfs = (StartOfs << 8) + (unsigned char) (pInfoBlock->header[0x82]);
	StartOfs = (StartOfs << 8) + (unsigned char) (pInfoBlock->header[0x83]);
	pInfoBlock->StartOfs = StartOfs;

	return CAP_Status_OK;
}


// Exported function.
// Seek to start of image file and read image header, extract & verify header contents, seek to start of image data.
int CAP_ReadHeader(HANDLE hHandle)
{
	size_t numread;
	int    ret;

	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, CAP_Status_Error_Invalid_Handle);
	ASSERT(pInfoBlock->fd != 0, CAP_Status_Error_File_not_open);

	if (CAP_SeekFile(hHandle, SEEK_START_OF_FILE) != CAP_Status_OK)
		return CAP_Status_Error_Seek_failed;

	if ((numread = fread(pInfoBlock->header, Default_CAP_Header_Size, 1, pInfoBlock->fd)) == 0)
		return CAP_Status_Error_Reading_header;

	pInfoBlock->header[Default_CAP_Header_Size] = 0; // 0-terminate header.

	if ((ret = Extract_and_Verify_CAP_Header_Contents(hHandle)) != CAP_Status_OK)
		return ret;

	if (CAP_SeekFile(hHandle, SEEK_START_OF_DATA) != CAP_Status_OK)
		return CAP_Status_Error_Seek_failed;

	return CAP_Status_OK;
}


// Internal function.
// Initialize a new image header from internal content representation.
int Update_CAP_Header_Contents(HANDLE hHandle)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, CAP_Status_Error_Invalid_Handle);

	memset(pInfoBlock->header, 0x00, sizeof(pInfoBlock->header));

	// Set CAP file signature.
	sprintf(&(pInfoBlock->header[0x00]), "%s", "TAPEIMAGE");

	// Set CAP file version.
	sprintf(&(pInfoBlock->header[0x10]), "%s", "v1.00");

	// Set timer precision.
	if (pInfoBlock->Precision == 16)
		sprintf(&(pInfoBlock->header[0x20]), "%s", "62.5ns");
	else if (pInfoBlock->Precision == 1)
		sprintf(&(pInfoBlock->header[0x20]), "%s", "1us");

	// Set machine.
	if (pInfoBlock->Machine == CAP_Machine_C64)
		sprintf(&(pInfoBlock->header[0x30]), "%s", "C64");
	else if (pInfoBlock->Machine == CAP_Machine_C16)
		sprintf(&(pInfoBlock->header[0x30]), "%s", "C16");
	else if (pInfoBlock->Machine == CAP_Machine_VC20)
		sprintf(&(pInfoBlock->header[0x30]), "%s", "VC20");
	else if (pInfoBlock->Machine == CAP_Machine_Spec48K)
		sprintf(&(pInfoBlock->header[0x30]), "%s", "Spectrum48K");
	else if (pInfoBlock->Machine == CAP_Machine_CUSTOM)
		sprintf(&(pInfoBlock->header[0x30]), "%s", "CUSTOM");

	// Set video.
	if (pInfoBlock->Video == CAP_Video_PAL)
		sprintf(&(pInfoBlock->header[0x40]), "%s", "PAL");
	else if (pInfoBlock->Video == CAP_Video_NTSC)
		sprintf(&(pInfoBlock->header[0x40]), "%s", "NTSC");
	else if (pInfoBlock->Video == CAP_Video_Spec48K)
		sprintf(&(pInfoBlock->header[0x40]), "%s", "Spectrum48K");
	else if (pInfoBlock->Video == CAP_Video_CUSTOM)
		sprintf(&(pInfoBlock->header[0x40]), "%s", "CUSTOM");

	// Check start signal edge.
	if (pInfoBlock->StartEdge == CAP_StartEdge_Falling)
		sprintf(&(pInfoBlock->header[0x50]), "%s", "FallingEdge");
	else if (pInfoBlock->StartEdge == CAP_StartEdge_Rising)
		sprintf(&(pInfoBlock->header[0x50]), "%s", "RisingEdge");

	// Check signal format.
	if (pInfoBlock->SignalFormat == CAP_SignalFormat_Relative)
		sprintf(&(pInfoBlock->header[0x60]), "%s", "Relative");
	else if (pInfoBlock->SignalFormat == CAP_SignalFormat_Absolute)
		sprintf(&(pInfoBlock->header[0x60]), "%s", "Absolute");

	// Only compatible with 40bit timestamps.
	sprintf(&(pInfoBlock->header[0x70]), "%s", "40bit");

	// Set data start offset.
	pInfoBlock->header[0x80] = (unsigned char) ((pInfoBlock->StartOfs & 0xff000000) >> 24);
	pInfoBlock->header[0x81] = (unsigned char) ((pInfoBlock->StartOfs & 0x00ff0000) >> 16);
	pInfoBlock->header[0x82] = (unsigned char) ((pInfoBlock->StartOfs & 0x0000ff00) >>  8);
	pInfoBlock->header[0x83] = (unsigned char) ((pInfoBlock->StartOfs & 0x000000ff)      );

	sprintf(&(pInfoBlock->header[0x90]), "%s", "----------------");

	return CAP_Status_OK;
}


// Exported function.
// Seek to start of file & write image header.
int CAP_WriteHeader(HANDLE hHandle)
{
	int ret;

	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, CAP_Status_Error_Invalid_Handle);
	ASSERT(pInfoBlock->fd != 0, CAP_Status_Error_File_not_open);

	if ((ret = Update_CAP_Header_Contents(hHandle)) != CAP_Status_OK)
		return ret;

	if (CAP_SeekFile(hHandle, SEEK_START_OF_FILE) != CAP_Status_OK)
		return CAP_Status_Error_Seek_failed;

	if (fwrite(pInfoBlock->header, Default_CAP_Header_Size, 1, pInfoBlock->fd) != 1)
		return CAP_Status_Error_Writing_header;

	return CAP_Status_OK;
}


// Exported function.
// Write addon string after image header.
int CAP_WriteHeaderAddon(HANDLE hHandle, unsigned char *pucString, unsigned int uiStringLen)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, CAP_Status_Error_Invalid_Handle);
	ASSERT(pInfoBlock->fd != 0, CAP_Status_Error_File_not_open);

	if (fwrite(pucString, uiStringLen, 1, pInfoBlock->fd) != 1)
		return CAP_Status_Error_Writing_header;

	return CAP_Status_OK;
}


// Exported function.
// Read a signal from image, increment byte counter.
int CAP_ReadSignal(HANDLE hHandle, unsigned __int64 *pui64Signal, int *piCounter)
{
	unsigned char    buf5[5]; // Compatible with 40bit signal width.
	unsigned __int64 ui64Signal;

	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, CAP_Status_Error_Invalid_Handle);
	ASSERT(pInfoBlock->fd != 0, CAP_Status_Error_File_not_open);
	ASSERT(pui64Signal != 0, CAP_Status_Error_Invalid_pointer);

	if (fread(buf5, 5, 1, pInfoBlock->fd) != 1)
	{
		if (feof(pInfoBlock->fd) == 0)
			return CAP_Status_Error_Reading_data;
		else
			return CAP_Status_OK_End_of_file;
	}

	ui64Signal = buf5[0];
	ui64Signal = (ui64Signal << 8) + buf5[1];
	ui64Signal = (ui64Signal << 8) + buf5[2];
	ui64Signal = (ui64Signal << 8) + buf5[3];
	ui64Signal = (ui64Signal << 8) + buf5[4];

	*pui64Signal = ui64Signal;

	if (piCounter != NULL)
		(*piCounter)+=5;

	return CAP_Status_OK;
}


// Internal function.
// Write a single byte to image, increment counter.
int CAP_WriteSingleByte(HANDLE hHandle, unsigned char ucByte, int *piCounter)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, CAP_Status_Error_Invalid_Handle);

	if (pInfoBlock->fd == NULL)
		return CAP_Status_Error_File_not_open;

	if (fwrite(&ucByte, 1, 1, pInfoBlock->fd) != 1)
		return CAP_Status_Error_Writing_data;

	if (piCounter != NULL)
		(*piCounter)++;

	return CAP_Status_OK;
}

// Exported function.
// Write a signal to image, increment counter for each written byte.
int CAP_WriteSignal(HANDLE hHandle, unsigned __int64 ui64Signal, int *piCounter)
{
	if (CAP_WriteSingleByte( hHandle, (unsigned char) ((ui64Signal >> 32) & 0xff), piCounter ) != CAP_Status_OK)
		return CAP_Status_Error_Writing_data;

	if (CAP_WriteSingleByte( hHandle, (unsigned char) ((ui64Signal >> 24) & 0xff), piCounter ) != CAP_Status_OK)
		return CAP_Status_Error_Writing_data;

	if (CAP_WriteSingleByte( hHandle, (unsigned char) ((ui64Signal >> 16) & 0xff), piCounter ) != CAP_Status_OK)
		return CAP_Status_Error_Writing_data;

	if (CAP_WriteSingleByte( hHandle, (unsigned char) ((ui64Signal >> 8) & 0xff), piCounter ) != CAP_Status_OK)
		return CAP_Status_Error_Writing_data;

	if (CAP_WriteSingleByte( hHandle, (unsigned char) ((ui64Signal) & 0xff), piCounter ) != CAP_Status_OK)
		return CAP_Status_Error_Writing_data;

	return CAP_Status_OK;
}


// Exported function.
// Verify header contents (Signature, Version, Precision, Machine, Video, StartEdge, SignalFormat, SignalWidth, StartOfs).
int CAP_isValidHeader(HANDLE hHandle)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, CAP_Status_Error_Invalid_Handle);

	// Check CAP file signature.
	if (strcmp(&(pInfoBlock->header[0x00]),"TAPEIMAGE") != 0)
		return CAP_Status_Error_Wrong_signature;

	// Check CAP file version.
	if (strcmp(&(pInfoBlock->header[0x10]),"v1.00") != 0)
		return CAP_Status_Error_Wrong_version;

	// Check timer precision.
	if (   (pInfoBlock->Precision != 16)
		&& (pInfoBlock->Precision != 1))
		return CAP_Status_Error_Wrong_precision;

	// Check machine.
	if (   (pInfoBlock->Machine != CAP_Machine_C64)
		&& (pInfoBlock->Machine != CAP_Machine_C16)
		&& (pInfoBlock->Machine != CAP_Machine_VC20)
		&& (pInfoBlock->Machine != CAP_Machine_Spec48K)
		&& (pInfoBlock->Machine != CAP_Machine_CUSTOM))
		return CAP_Status_Error_Wrong_machine;

	// Check video.
	if (   (pInfoBlock->Video != CAP_Video_PAL)
		&& (pInfoBlock->Video != CAP_Video_NTSC)
		&& (pInfoBlock->Video != CAP_Video_Spec48K)
		&& (pInfoBlock->Video != CAP_Video_CUSTOM))
		return CAP_Status_Error_Wrong_video;

	// Check start signal edge.
	if (   (pInfoBlock->StartEdge != CAP_StartEdge_Falling)
		&& (pInfoBlock->StartEdge != CAP_StartEdge_Rising))
		return CAP_Status_Error_Wrong_start_signal_edge;

	// Check signal format.
	if (   (pInfoBlock->SignalFormat != CAP_SignalFormat_Relative)
		&& (pInfoBlock->SignalFormat != CAP_SignalFormat_Absolute))
		return CAP_Status_Error_Wrong_signal_format;

	// Check signal width.
	if (pInfoBlock->SignalWidth != CAP_SignalWidth_40bit)
		return CAP_Status_Error_Wrong_signal_width;

	// Check data start offset.
	if (pInfoBlock->StartOfs < Default_CAP_Header_Size)
		return CAP_Status_Error_Wrong_data_start_offset;

	return CAP_Status_OK;
}


// Exported function.
// Return timestamp precision from header.
int CAP_GetHeader_Precision(HANDLE hHandle, unsigned int *puiPrecision)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, CAP_Status_Error_Invalid_Handle);
	ASSERT(puiPrecision != 0, CAP_Status_Error_Invalid_pointer);

	*puiPrecision = pInfoBlock->Precision;
	return CAP_Status_OK;
}


// Exported function.
// Set timestamp precision in header.
int CAP_SetHeader_Precision(HANDLE hHandle, unsigned int uiPrecision)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, CAP_Status_Error_Invalid_Handle);

	pInfoBlock->Precision = uiPrecision;
	return CAP_Status_OK;
}


// Exported function.
// Return target machine type from header.
int CAP_GetHeader_Machine(HANDLE hHandle, unsigned char *pucMachine)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, CAP_Status_Error_Invalid_Handle);
	ASSERT(pucMachine != 0, CAP_Status_Error_Invalid_pointer);

	*pucMachine = pInfoBlock->Machine;
	return CAP_Status_OK;
}


// Exported function.
// Set target machine type in header.
int CAP_SetHeader_Machine(HANDLE hHandle, unsigned char ucMachine)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, CAP_Status_Error_Invalid_Handle);

	pInfoBlock->Machine = ucMachine;
	return CAP_Status_OK;
}


// Exported function.
// Return target video type from header.
int CAP_GetHeader_Video(HANDLE hHandle, unsigned char *pucVideo)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, CAP_Status_Error_Invalid_Handle);
	ASSERT(pucVideo != 0, CAP_Status_Error_Invalid_pointer);

	*pucVideo = pInfoBlock->Video;
	return CAP_Status_OK;
}


// Exported function.
// Set target video type in header.
int CAP_SetHeader_Video(HANDLE hHandle, unsigned char ucVideo)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, CAP_Status_Error_Invalid_Handle);

	pInfoBlock->Video = ucVideo;
	return CAP_Status_OK;
}


// Exported function.
// Return start signal edge from header.
int CAP_GetHeader_StartEdge(HANDLE hHandle, unsigned char *pucStartEdge)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, CAP_Status_Error_Invalid_Handle);
	ASSERT(pucStartEdge != 0, CAP_Status_Error_Invalid_pointer);

	*pucStartEdge = pInfoBlock->StartEdge;
	return CAP_Status_OK;
}


// Exported function.
// Set start signal edge in header.
int CAP_SetHeader_StartEdge(HANDLE hHandle, unsigned char ucStartEdge)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, CAP_Status_Error_Invalid_Handle);

	pInfoBlock->StartEdge = ucStartEdge;
	return CAP_Status_OK;
}


// Exported function.
int CAP_GetHeader_SignalFormat(HANDLE hHandle, unsigned char *pucSignalFormat)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, CAP_Status_Error_Invalid_Handle);
	ASSERT(pucSignalFormat != 0, CAP_Status_Error_Invalid_pointer);

	*pucSignalFormat = pInfoBlock->SignalFormat;
	return CAP_Status_OK;
}


// Exported function.
// Return signal format from header.
int CAP_SetHeader_SignalFormat(HANDLE hHandle, unsigned char ucSignalFormat)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, CAP_Status_Error_Invalid_Handle);

	pInfoBlock->SignalFormat = ucSignalFormat;
	return CAP_Status_OK;
}


// Exported function.
// Set signal width in header.
int CAP_GetHeader_SignalWidth(HANDLE hHandle, unsigned int *puiSignalWidth)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, CAP_Status_Error_Invalid_Handle);
	ASSERT(puiSignalWidth != 0, CAP_Status_Error_Invalid_pointer);

	*puiSignalWidth = pInfoBlock->SignalWidth;
	return CAP_Status_OK;
}


// Exported function.
// Return signal width from header.
int CAP_SetHeader_SignalWidth(HANDLE hHandle, unsigned int uiSignalWidth)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, CAP_Status_Error_Invalid_Handle);

	pInfoBlock->SignalWidth = uiSignalWidth;
	return CAP_Status_OK;
}


// Exported function.
// Set data start offset in header.
int CAP_SetHeader_StartOffset(HANDLE hHandle, unsigned int uiStartOffset)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, CAP_Status_Error_Invalid_Handle);

	pInfoBlock->StartOfs = uiStartOffset;
	return CAP_Status_OK;
}


// Exported function.
// Get all header entries at once.
int CAP_GetHeader(HANDLE        hHandle,
                  unsigned int  *puiPrecision,
                  unsigned char *pucMachine,
                  unsigned char *pucVideo,
                  unsigned char *pucStartEdge,
                  unsigned char *pucSignalFormat,
                  unsigned int  *puiSignalWidth,
                  unsigned int  *puiStartOffset)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, CAP_Status_Error_Invalid_Handle);

	if ((puiPrecision == NULL) || (pucMachine == NULL) || (pucVideo == NULL) || (pucStartEdge == NULL) || (pucSignalFormat == NULL) || (puiSignalWidth == NULL) || (puiStartOffset == NULL))
		return CAP_Status_Error_Invalid_pointer;

	*puiPrecision    = pInfoBlock->Precision;
	*pucMachine      = pInfoBlock->Machine;
	*pucVideo        = pInfoBlock->Video;
	*pucStartEdge    = pInfoBlock->StartEdge;
	*pucSignalFormat = pInfoBlock->SignalFormat;
	*puiSignalWidth  = pInfoBlock->SignalWidth;
	*puiStartOffset  = pInfoBlock->StartOfs;

	return CAP_Status_OK;
}


// Exported function.
// Set all header entries at once.
int CAP_SetHeader(HANDLE        hHandle,
                  unsigned int  uiPrecision,
                  unsigned char ucMachine,
                  unsigned char ucVideo,
                  unsigned char ucStartEdge,
                  unsigned char ucSignalFormat,
                  unsigned int  uiSignalWidth,
                  unsigned int  uiStartOffset)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, CAP_Status_Error_Invalid_Handle);

	pInfoBlock->Precision    = uiPrecision;
	pInfoBlock->Machine      = ucMachine;
	pInfoBlock->Video        = ucVideo;
	pInfoBlock->StartEdge    = ucStartEdge;
	pInfoBlock->SignalFormat = ucSignalFormat;
	pInfoBlock->SignalWidth  = uiSignalWidth;
	pInfoBlock->StartOfs     = uiStartOffset;

	return CAP_Status_OK;
}


// Exported function.
// Outputs info on error status to console.
void CAP_OutputError(int Status)
{
	switch (Status)
	{
		case CAP_Status_Error_Invalid_Handle:
			printf("Invalid CAP file handle.\n");
			break;
		case CAP_Status_Error_Invalid_pointer:
			printf("Invalid pointer in CAP handling.\n");
			break;
		case CAP_Status_Error_Out_of_memory:
			printf("Out of memory in CAP handling.\n");
			break;
		case CAP_Status_Error_Creating_file:
			printf("Can't create CAP file.\n");
			break;
		case CAP_Status_Error_File_not_found:
			printf("CAP file not found.\n");
			break;
		case CAP_Status_Error_Closing_file:
			printf("Closing CAP file failed.\n");
			break;
		case CAP_Status_Error_File_not_open:
			printf("CAP file not open.\n");
			break;
		case CAP_Status_Error_Seek_wrong_destination:
			printf("Illegal CAP seek destination.\n");
			break;
		case CAP_Status_Error_Seek_failed:
			printf("CAP seek failed.\n");
			break;
		case CAP_Status_Error_Reading_header:
			printf("Read CAP header failed.\n");
			break;
		case CAP_Status_Error_Writing_header:
			printf("Write CAP header failed.\n");
			break;
		case CAP_Status_Error_Reading_data:
			printf("Reading CAP data failed.\n");
			break;
		case CAP_Status_Error_Writing_data:
			printf("Writing CAP data failed.\n");
			break;
		case CAP_Status_Error_Wrong_signature:
			printf("Illegal CAP image signature.\n");
			break;
		case CAP_Status_Error_Wrong_version:
			printf("Illegal CAP image version.\n");
			break;
		case CAP_Status_Error_Wrong_machine:
			printf("Illegal target machine in CAP header.\n");
			break;
		case CAP_Status_Error_Wrong_video:
			printf("Illegal target video in CAP header.\n");
			break;
		case CAP_Status_Error_Wrong_precision:
			printf("Illegal precision in CAP header.\n");
			break;
		case CAP_Status_Error_Wrong_start_signal_edge:
			printf("Illegal start signal edge in CAP header.\n");
			break;
		case CAP_Status_Error_Wrong_signal_format:
			printf("Illegal signal format in CAP header.\n");
			break;
		case CAP_Status_Error_Wrong_signal_width:
			printf("Illegal signal width in CAP header.\n");
			break;
		case CAP_Status_Error_Wrong_data_start_offset:
			printf("Illegal data start offset in CAP header.\n");
			break;
		default:
			printf("Unknown error (%d)\n", Status);
			break;
	}
}
