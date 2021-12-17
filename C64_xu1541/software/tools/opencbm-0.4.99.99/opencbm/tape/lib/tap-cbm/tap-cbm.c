/*
 *  CBM 1530/1531 tape routines.
 *  Copyright 2012 Arnd Menge, arnd(at)jonnz(dot)de
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include <malloc.h>

#include "tap-cbm.h"

#define Header_Size_TAP_CBM 0x14

#define SEEK_START_OF_FILE 1
#define SEEK_START_OF_DATA 2

#define DETAILED_INFO(rv) {fprintf(stderr, "Error : %d\nModule: %s\nBuilt : %s %s\nLine  : %d\n", rv, __FILE__, __DATE__, __TIME__, __LINE__);}

#define ASSERT(x, rv) {if (!x) {DETAILED_INFO(rv); return rv;}}

typedef struct _INFOBLOCK {
	unsigned int  MemTag;
	FILE          *fd;
	char          header[Header_Size_TAP_CBM+1]; // + 0-termination
	unsigned char Machine, Video, TAPversion;
	unsigned int  ByteCount;
	unsigned int  MemTag2;
} INFOBLOCK, *PINFOBLOCK;


// Exported function.
// Create (overwrite) an image file for writing.
int TAP_CBM_CreateFile(HANDLE *hHandle, char *pcFilename)
{
	PINFOBLOCK pInfoBlock;

	ASSERT(hHandle != 0, TAP_CBM_Status_Error_Invalid_Handle);

	pInfoBlock = (struct _INFOBLOCK*)malloc(sizeof(INFOBLOCK));

	ASSERT(pInfoBlock != 0, TAP_CBM_Status_Error_Out_of_memory);

	memset(pInfoBlock, 0x00, sizeof(INFOBLOCK));

	// Write memory tags.
	pInfoBlock->MemTag  = 0x5f424954; // TIB_
	pInfoBlock->MemTag2 = 0x5449425f; // _BIT

	pInfoBlock->fd = fopen(pcFilename, "wb");
	if (pInfoBlock->fd == NULL)
	{
		free(pInfoBlock);
		return TAP_CBM_Status_Error_Creating_file;
	}

	*hHandle = (HANDLE) pInfoBlock;

	return TAP_CBM_Status_OK;
}


// Exported function.
// Open an existing image file for reading.
int TAP_CBM_OpenFile(HANDLE *hHandle, char *pcFilename)
{
	PINFOBLOCK pInfoBlock;

	ASSERT(hHandle != 0, TAP_CBM_Status_Error_Invalid_Handle);

	pInfoBlock = (struct _INFOBLOCK*)malloc(sizeof(INFOBLOCK));

	ASSERT(pInfoBlock != 0, TAP_CBM_Status_Error_Out_of_memory);

	memset(pInfoBlock, 0x00, sizeof(INFOBLOCK));

	// Write memory tags.
	pInfoBlock->MemTag  = 0x5f424954; // TIB_
	pInfoBlock->MemTag2 = 0x5449425f; // _BIT

	pInfoBlock->fd = fopen(pcFilename, "rb");
	if (pInfoBlock->fd == NULL)
	{
		free(pInfoBlock);
		return TAP_CBM_Status_Error_File_not_found;
	}

	*hHandle = (HANDLE) pInfoBlock;

	return TAP_CBM_Status_OK;
}


// Exported function.
// Close an image file.
int TAP_CBM_CloseFile(HANDLE *hHandle)
{
	PINFOBLOCK pInfoBlock;

	ASSERT(hHandle != 0, TAP_CBM_Status_Error_Invalid_Handle);

	pInfoBlock = (struct _INFOBLOCK*)(*hHandle);

	ASSERT(pInfoBlock != 0, TAP_CBM_Status_Error_Invalid_Handle);

	if (pInfoBlock->fd != NULL)
		if (fclose(pInfoBlock->fd) != 0)
			return TAP_CBM_Status_Error_Closing_file;

	free(pInfoBlock);

	*hHandle = NULL;

	return TAP_CBM_Status_OK;
}


// Exported function.
// Check if a file is already existing.
int TAP_CBM_isFilePresent(char *pcFilename)
{
	FILE *fd = fopen(pcFilename, "r");
	if (fd == NULL)
		return TAP_CBM_Status_Error_File_not_found;

	fclose(fd);
	return TAP_CBM_Status_OK;
}


// Exported function.
// Return file size of image file (moves file pointer).
int TAP_CBM_GetFileSize(HANDLE hHandle, int *piFileSize)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, TAP_CBM_Status_Error_Invalid_Handle);
	ASSERT(piFileSize != 0, TAP_CBM_Status_Error_Invalid_pointer);
	ASSERT(pInfoBlock->fd != 0, TAP_CBM_Status_Error_File_not_open);

	if (fseek(pInfoBlock->fd, 0, SEEK_END) != 0)
		return TAP_CBM_Status_Error_Seek_failed;

	*piFileSize = ftell(pInfoBlock->fd);
	rewind(pInfoBlock->fd);

	return TAP_CBM_Status_OK;
}


// Internal function.
// Seek to start of image file, or seek start of image data.
int TAP_CBM_SeekFile(HANDLE hHandle, char cDestination)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, TAP_CBM_Status_Error_Invalid_Handle);
	ASSERT(pInfoBlock->fd != 0, TAP_CBM_Status_Error_File_not_open);

	if (cDestination == SEEK_START_OF_FILE)
	{
		if (fseek(pInfoBlock->fd, 0, SEEK_SET) != 0)
			return TAP_CBM_Status_Error_Seek_failed;
	}
	else if (cDestination == SEEK_START_OF_DATA)
	{
		if (fseek(pInfoBlock->fd, Header_Size_TAP_CBM, SEEK_SET) != 0)
			return TAP_CBM_Status_Error_Seek_failed;
	}

	return TAP_CBM_Status_OK;
}


// Internal function.
// Extract & verify header contents.
int Extract_and_Verify_TAP_Header_Contents(HANDLE hHandle)
{
	unsigned int ByteCount;

	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, TAP_CBM_Status_Error_Invalid_Handle);

	// Check TAPversion.
	pInfoBlock->TAPversion = pInfoBlock->header[0xc];
	if (pInfoBlock->TAPversion > 2)
		return TAP_CBM_Status_Error_Wrong_version;

	pInfoBlock->header[0xc] = 0; // 0-termination.

	// Check TAP file signature.
	if (strcmp(&(pInfoBlock->header[0x00]),"C64-TAPE-RAW") == 0)
	{
		// Check computer type.
		if (pInfoBlock->header[0xd] == TAP_Machine_C64)
			pInfoBlock->Machine = TAP_Machine_C64;
		else if (pInfoBlock->header[0xd] == TAP_Machine_VC20)
			pInfoBlock->Machine = TAP_Machine_VC20;
		else
			return TAP_CBM_Status_Error_Wrong_machine;
	}
	else if (strcmp(&(pInfoBlock->header[0x00]),"C16-TAPE-RAW") == 0)
	{
		// Check computer type.
		if (pInfoBlock->header[0xd] == TAP_Machine_C16)
			pInfoBlock->Machine = TAP_Machine_C16;
		else
			return TAP_CBM_Status_Error_Wrong_machine;
	}
	else
		return TAP_CBM_Status_Error_Wrong_signature;

	// Check video type.
	if (pInfoBlock->header[0xe] == TAP_Video_PAL)
		pInfoBlock->Video = TAP_Video_PAL;
	else if (pInfoBlock->header[0xe] == TAP_Video_NTSC)
		pInfoBlock->Video = TAP_Video_NTSC;
	else
		return TAP_CBM_Status_Error_Wrong_video;

	// Extract signal number (sum of all signal bytes).
	ByteCount = (unsigned char) (pInfoBlock->header[0x10]);
	ByteCount = (ByteCount << 8) + (unsigned char) (pInfoBlock->header[0x11]);
	ByteCount = (ByteCount << 8) + (unsigned char) (pInfoBlock->header[0x12]);
	ByteCount = (ByteCount << 8) + (unsigned char) (pInfoBlock->header[0x13]);
	pInfoBlock->ByteCount = ByteCount;

	return TAP_CBM_Status_OK;
}


// Exported function.
// Seek to start of image file and read image header, extract & verify header contents.
int TAP_CBM_ReadHeader(HANDLE hHandle)
{
	size_t numread;
	int    ret;

	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, TAP_CBM_Status_Error_Invalid_Handle);
	ASSERT(pInfoBlock->fd != 0, TAP_CBM_Status_Error_File_not_open);

	if (TAP_CBM_SeekFile(hHandle, SEEK_START_OF_FILE) != TAP_CBM_Status_OK)
		return TAP_CBM_Status_Error_Seek_failed;

	if ((numread = fread(pInfoBlock->header, Header_Size_TAP_CBM, 1, pInfoBlock->fd)) == 0)
		return TAP_CBM_Status_Error_Reading_header;

	if ((ret = Extract_and_Verify_TAP_Header_Contents(hHandle)) != TAP_CBM_Status_OK)
		return ret;

	return TAP_CBM_Status_OK;
}


// Internal function.
// Initialize a new image header from internal content representation.
int Update_TAP_CBM_Header_Contents(HANDLE hHandle)
{
	unsigned int tap_counter;

	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, TAP_CBM_Status_Error_Invalid_Handle);

	memset(pInfoBlock->header, 0x00, sizeof(pInfoBlock->header));

	// Set TAP file signature.
	if (pInfoBlock->Machine == TAP_Machine_C64)
		sprintf(&(pInfoBlock->header[0x00]), "%s", "C64-TAPE-RAW");
	else if (pInfoBlock->Machine == TAP_Machine_VC20)
		sprintf(&(pInfoBlock->header[0x00]), "%s", "C64-TAPE-RAW");
	else if (pInfoBlock->Machine == TAP_Machine_C16)
		sprintf(&(pInfoBlock->header[0x00]), "%s", "C16-TAPE-RAW");

	pInfoBlock->header[0xc] = pInfoBlock->TAPversion;
	pInfoBlock->header[0xd] = pInfoBlock->Machine;
	pInfoBlock->header[0xe] = pInfoBlock->Video;

	tap_counter = pInfoBlock->ByteCount;
	pInfoBlock->header[0x10] = (tap_counter & 0xff); tap_counter >>= 8;
	pInfoBlock->header[0x11] = (tap_counter & 0xff); tap_counter >>= 8;
	pInfoBlock->header[0x12] = (tap_counter & 0xff); tap_counter >>= 8;
	pInfoBlock->header[0x13] = (tap_counter & 0xff);

	return TAP_CBM_Status_OK;
}


// Exported function.
// Seek to start of file & write image header.
int TAP_CBM_WriteHeader(HANDLE hHandle)
{
	int ret;

	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, TAP_CBM_Status_Error_Invalid_Handle);
	ASSERT(pInfoBlock->fd != 0, TAP_CBM_Status_Error_File_not_open);

	if ((ret = Update_TAP_CBM_Header_Contents(hHandle)) != TAP_CBM_Status_OK)
		return ret;

	if (TAP_CBM_SeekFile(hHandle, SEEK_START_OF_FILE) != TAP_CBM_Status_OK)
		return TAP_CBM_Status_Error_Seek_failed;

	if (fwrite(pInfoBlock->header, Header_Size_TAP_CBM, 1, pInfoBlock->fd) != 1)
		return TAP_CBM_Status_Error_Writing_header;

	return TAP_CBM_Status_OK;
}


// Exported function.
// Read a signal from image, increment counter for each read byte.
int TAP_CBM_ReadSignal(HANDLE hHandle, unsigned int *puiSignal, unsigned int *puiCounter)
{
	unsigned char buf3[3];
	unsigned char ch;
	unsigned int  uiSignal;

	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, TAP_CBM_Status_Error_Invalid_Handle);
	ASSERT(pInfoBlock->fd != 0, TAP_CBM_Status_Error_File_not_open);
	ASSERT(puiSignal != 0, TAP_CBM_Status_Error_Invalid_pointer);
	ASSERT(puiCounter != 0, TAP_CBM_Status_Error_Invalid_pointer);

	if (fread(&ch, 1, 1, pInfoBlock->fd) != 1)
	{
		if (feof(pInfoBlock->fd) == 0)
			return TAP_CBM_Status_Error_Reading_data;
		else
			return TAP_CBM_Status_OK_End_of_file;
	}

	if (ch == 0) // Pause detected.
	{
		if (pInfoBlock->TAPversion == TAPv0)
			*puiSignal = 2040; // 8*0xff=2040
		else
		{
			if (fread(buf3, 3, 1, pInfoBlock->fd) != 1)
			{
				if (feof(pInfoBlock->fd) == 0)
					return TAP_CBM_Status_Error_Reading_data;
				else
					return TAP_CBM_Status_OK_End_of_file;
			}

			(*puiCounter)+=3;

			uiSignal = buf3[2];
			uiSignal = (uiSignal << 8) | buf3[1];
			uiSignal = (uiSignal << 8) | buf3[0];
			*puiSignal = uiSignal;
		}
	}
	else // Data detected.
		*puiSignal = ((unsigned int)ch)*8;

	(*puiCounter)++;

	return TAP_CBM_Status_OK;
}


// Exported function.
// Write a single unsigned char to image file.
int TAP_CBM_WriteSignal_1Byte(HANDLE hHandle, unsigned char ucByte, unsigned int *puiCounter)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, TAP_CBM_Status_Error_Invalid_Handle);
	ASSERT(pInfoBlock->fd != 0, TAP_CBM_Status_Error_File_not_open);
	ASSERT(puiCounter != 0, TAP_CBM_Status_Error_Invalid_pointer);

	if (fwrite(&ucByte, 1, 1, pInfoBlock->fd) != 1)
		return TAP_CBM_Status_Error_Writing_data;

	(*puiCounter)++;

	return TAP_CBM_Status_OK;
}


// Exported function.
// Write 32bit unsigned integer to image file: LSB first, MSB last.
int TAP_CBM_WriteSignal_4Bytes(HANDLE hHandle, unsigned int uiSignal, unsigned int *puiCounter)
{
	int ret;

	if ((ret = TAP_CBM_WriteSignal_1Byte(hHandle, (uiSignal      ) & 0xff, puiCounter)) != TAP_CBM_Status_OK)
		return ret;

	if ((ret = TAP_CBM_WriteSignal_1Byte(hHandle, (uiSignal >>  8) & 0xff, puiCounter)) != TAP_CBM_Status_OK)
		return ret;

	if ((ret = TAP_CBM_WriteSignal_1Byte(hHandle, (uiSignal >> 16) & 0xff, puiCounter)) != TAP_CBM_Status_OK)
		return ret;

	if ((ret = TAP_CBM_WriteSignal_1Byte(hHandle, (uiSignal >> 24) & 0xff, puiCounter)) != TAP_CBM_Status_OK)
		return ret;

	return TAP_CBM_Status_OK;
}


// Exported function.
// Verify header contents (Signature, Machine, Video, TAPversion).
int TAP_CBM_isValidHeader(HANDLE hHandle)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, TAP_CBM_Status_Error_Invalid_Handle);

	// Check TAPversion.
	if (pInfoBlock->TAPversion > 2)
		return TAP_CBM_Status_Error_Wrong_version;

	// Check TAP file signature.
	if (strcmp(&(pInfoBlock->header[0x00]),"C64-TAPE-RAW") == 0)
	{
		// Check computer type.
		if (   (pInfoBlock->Machine != TAP_Machine_C64)
			&& (pInfoBlock->Machine != TAP_Machine_VC20))
			return TAP_CBM_Status_Error_Wrong_machine;
	}
	else if (strcmp(&(pInfoBlock->header[0x00]),"C16-TAPE-RAW") == 0)
	{
		// Check computer type.
		if (pInfoBlock->Machine != TAP_Machine_C16)
			return TAP_CBM_Status_Error_Wrong_machine;
	}
	else
		return TAP_CBM_Status_Error_Wrong_signature;

	// Check video type.
	if (   (pInfoBlock->Video != TAP_Video_PAL)
		&& (pInfoBlock->Video != TAP_Video_NTSC))
		return TAP_CBM_Status_Error_Wrong_video;

	return TAP_CBM_Status_OK;
}


// Exported function.
// Return target machine type from header.
int TAP_CBM_GetHeader_Machine(HANDLE hHandle, unsigned char *pucMachine)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, TAP_CBM_Status_Error_Invalid_Handle);
	ASSERT(pucMachine != 0, TAP_CBM_Status_Error_Invalid_pointer);

	*pucMachine = pInfoBlock->Machine;
	return TAP_CBM_Status_OK;
}


// Exported function.
// Set target machine type in header.
int TAP_CBM_SetHeader_Machine(HANDLE hHandle, unsigned char ucMachine)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, TAP_CBM_Status_Error_Invalid_Handle);

	pInfoBlock->Machine = ucMachine;
	return TAP_CBM_Status_OK;
}


// Exported function.
// Return target video type from header.
int TAP_CBM_GetHeader_Video(HANDLE hHandle, unsigned char *pucVideo)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, TAP_CBM_Status_Error_Invalid_Handle);
	ASSERT(pucVideo != 0, TAP_CBM_Status_Error_Invalid_pointer);

	*pucVideo = pInfoBlock->Video;
	return TAP_CBM_Status_OK;
}


// Exported function.
// Set target video type in header.
int TAP_CBM_SetHeader_Video(HANDLE hHandle, unsigned char ucVideo)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, TAP_CBM_Status_Error_Invalid_Handle);

	pInfoBlock->Video = ucVideo;
	return TAP_CBM_Status_OK;
}


// Exported function.
// Return TAP version from header.
int TAP_CBM_GetHeader_TAPversion(HANDLE hHandle, unsigned char *pucTAPversion)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, TAP_CBM_Status_Error_Invalid_Handle);
	ASSERT(pucTAPversion != 0, TAP_CBM_Status_Error_Invalid_pointer);

	*pucTAPversion = pInfoBlock->TAPversion;
	return TAP_CBM_Status_OK;
}


// Exported function.
// Set TAP version in header.
int TAP_CBM_SetHeader_TAPversion(HANDLE hHandle, unsigned char ucTAPversion)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, TAP_CBM_Status_Error_Invalid_Handle);

	pInfoBlock->TAPversion = ucTAPversion;
	return TAP_CBM_Status_OK;
}


// Exported function.
// Get signal byte count from header (sum of all signal bytes).
int TAP_CBM_GetHeader_ByteCount(HANDLE hHandle, unsigned int *puiByteCount)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, TAP_CBM_Status_Error_Invalid_Handle);
	ASSERT(puiByteCount != 0, TAP_CBM_Status_Error_Invalid_pointer);

	*puiByteCount = pInfoBlock->ByteCount;
	return TAP_CBM_Status_OK;
}


// Exported function.
// Set signal byte count in header (sum of all signal bytes).
int TAP_CBM_SetHeader_ByteCount(HANDLE hHandle, unsigned int uiByteCount)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, TAP_CBM_Status_Error_Invalid_Handle);

	pInfoBlock->ByteCount = uiByteCount;
	return TAP_CBM_Status_OK;
}


// Exported function.
// Get all header entries at once.
int TAP_CBM_GetHeader(HANDLE        hHandle,
                      unsigned char *pucMachine,
                      unsigned char *pucVideo,
                      unsigned char *pucTAPversion,
                      unsigned int  *puiByteCount)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, TAP_CBM_Status_Error_Invalid_Handle);

	if ((pucMachine == NULL) || (pucVideo == NULL) || (pucTAPversion == NULL) || (puiByteCount == NULL))
		return TAP_CBM_Status_Error_Invalid_pointer;

	*pucMachine    = pInfoBlock->Machine;
	*pucVideo      = pInfoBlock->Video;
	*pucTAPversion = pInfoBlock->TAPversion;
	*puiByteCount  = pInfoBlock->ByteCount;

	return TAP_CBM_Status_OK;
}


// Exported function.
// Set all header entries at once.
int TAP_CBM_SetHeader(HANDLE        hHandle,
                      unsigned char ucMachine,
                      unsigned char ucVideo,
                      unsigned char ucTAPversion,
                      unsigned int  uiByteCount)
{
	PINFOBLOCK pInfoBlock = (struct _INFOBLOCK*)hHandle;

	ASSERT(pInfoBlock != 0, TAP_CBM_Status_Error_Invalid_Handle);

	pInfoBlock->Machine    = ucMachine;
	pInfoBlock->Video      = ucVideo;
	pInfoBlock->TAPversion = ucTAPversion;
	pInfoBlock->ByteCount  = uiByteCount;

	return TAP_CBM_Status_OK;
}


// Exported function.
// Outputs info on error status to console.
void TAP_CBM_OutputError(int Status)
{
	switch (Status)
	{
		case TAP_CBM_Status_Error_Invalid_Handle:
			printf("Invalid TAP file handle.\n");
			break;
		case TAP_CBM_Status_Error_Invalid_pointer:
			printf("Invalid pointer in TAP handling.\n");
			break;
		case TAP_CBM_Status_Error_Out_of_memory:
			printf("Out of memory in TAP handling.\n");
			break;
		case TAP_CBM_Status_Error_Creating_file:
			printf("Can't create TAP file.\n");
			break;
		case TAP_CBM_Status_Error_File_not_found:
			printf("TAP file not found.\n");
			break;
		case TAP_CBM_Status_Error_Closing_file:
			printf("Closing TAP file failed.\n");
			break;
		case TAP_CBM_Status_Error_File_not_open:
			printf("TAP file not open.\n");
			break;
		case TAP_CBM_Status_Error_Seek_failed:
			printf("TAP seek failed.\n");
			break;
		case TAP_CBM_Status_Error_Reading_header:
			printf("Read TAP header failed.\n");
			break;
		case TAP_CBM_Status_Error_Writing_header:
			printf("Write TAP header failed.\n");
			break;
		case TAP_CBM_Status_Error_Reading_data:
			printf("Reading TAP data failed.\n");
			break;
		case TAP_CBM_Status_Error_Writing_data:
			printf("Writing TAP data failed.\n");
			break;
		case TAP_CBM_Status_Error_Wrong_signature:
			printf("Illegal TAP image signature.\n");
			break;
		case TAP_CBM_Status_Error_Wrong_version:
			printf("Illegal TAP image version.\n");
			break;
		case TAP_CBM_Status_Error_Wrong_machine:
			printf("Illegal target machine in TAP header.\n");
			break;
		case TAP_CBM_Status_Error_Wrong_video:
			printf("Illegal target video in TAP header.\n");
			break;
		default:
			printf("Unknown error (%d)\n", Status);
			break;
	}
}
