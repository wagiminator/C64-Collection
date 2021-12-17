/*
 *  CBM 1530/1531 tape routines.
 *  Copyright 2012 Arnd Menge, arnd(at)jonnz(dot)de
*/

#ifndef __TAP_CBM_H_
#define __TAP_CBM_H_

#include <Windows.h>

// Status results from exported functions
#define TAP_CBM_Status_OK                     0
#define TAP_CBM_Status_OK_End_of_file         1
#define TAP_CBM_Status_Error_Invalid_Handle  -1
#define TAP_CBM_Status_Error_Invalid_pointer -2
#define TAP_CBM_Status_Error_Out_of_memory   -3
#define TAP_CBM_Status_Error_Creating_file   -4
#define TAP_CBM_Status_Error_File_not_found  -5
#define TAP_CBM_Status_Error_Closing_file    -6
#define TAP_CBM_Status_Error_File_not_open   -7
#define TAP_CBM_Status_Error_Seek_failed     -9
#define TAP_CBM_Status_Error_Reading_header  -10
#define TAP_CBM_Status_Error_Writing_header  -11
#define TAP_CBM_Status_Error_Reading_data    -12
#define TAP_CBM_Status_Error_Writing_data    -13
#define TAP_CBM_Status_Error_Wrong_signature -14
#define TAP_CBM_Status_Error_Wrong_version   -15
#define TAP_CBM_Status_Error_Wrong_machine   -16
#define TAP_CBM_Status_Error_Wrong_video     -17

// Possible target machines for tape image
#define TAP_Machine_C64  0
#define TAP_Machine_VC20 1
#define TAP_Machine_C16  2

// Possible target video types for tape image
#define TAP_Video_PAL  0
#define TAP_Video_NTSC 1

// Possible tape image format versions
#define TAPv0 0
#define TAPv1 1
#define TAPv2 2

// Create (overwrite) an image file for writing.
int TAP_CBM_CreateFile(HANDLE *hHandle, char *pcFilename);

// Open an existing image file for reading.
int TAP_CBM_OpenFile(HANDLE *hHandle, char *pcFilename);

// Close an image file.
int TAP_CBM_CloseFile(HANDLE *hHandle);

// Check if a file is already existing.
int TAP_CBM_isFilePresent(char *pcFilename);

// Return file size of image file (moves file pointer).
int TAP_CBM_GetFileSize(HANDLE hHandle, int *piFileSize);

// Seek to start of image file and read image header, extract & verify header contents.
int TAP_CBM_ReadHeader(HANDLE hHandle);

// Seek to start of file & write image header.
int TAP_CBM_WriteHeader(HANDLE hHandle);

// Read a signal from image, increment counter for each read byte.
int TAP_CBM_ReadSignal(HANDLE hHandle, unsigned int *puiSignal, unsigned int *puiCounter);

// Write a single unsigned char to image file.
int TAP_CBM_WriteSignal_1Byte(HANDLE hHandle, unsigned char ucByte, unsigned int *puiCounter);

// Write 32bit unsigned integer to image file: LSB first, MSB last.
int TAP_CBM_WriteSignal_4Bytes(HANDLE hHandle, unsigned int uiSignal, unsigned int *puiCounter);

// Verify header contents (Signature, Machine, Video, TAPversion).
int TAP_CBM_isValidHeader(HANDLE hHandle);

// Return target machine type from header.
int TAP_CBM_GetHeader_Machine(HANDLE hHandle, unsigned char *pucMachine);

// Set target machine type in header.
int TAP_CBM_SetHeader_Machine(HANDLE hHandle, unsigned char ucMachine);

// Return target video type from header.
int TAP_CBM_GetHeader_Video(HANDLE hHandle, unsigned char *pucVideo);

// Set target video type in header.
int TAP_CBM_SetHeader_Video(HANDLE hHandle, unsigned char ucVideo);

// Return TAP version from header.
int TAP_CBM_GetHeader_TAPversion(HANDLE hHandle, unsigned char *pucTAPversion);

// Set TAP version in header.
int TAP_CBM_SetHeader_TAPversion(HANDLE hHandle, unsigned char ucTAPversion);

// Get signal byte count from header (sum of all signal bytes).
int TAP_CBM_GetHeader_ByteCount(HANDLE hHandle, unsigned int *puiByteCount);

// Set signal byte count in header (sum of all signal bytes).
int TAP_CBM_SetHeader_ByteCount(HANDLE hHandle, unsigned int uiByteCount);

// Get all header entries at once.
int TAP_CBM_GetHeader(HANDLE        hHandle,
                      unsigned char *pucMachine,
                      unsigned char *pucVideo,
                      unsigned char *pucTAPversion,
                      unsigned int  *puiByteCount);

// Set all header entries at once.
int TAP_CBM_SetHeader(HANDLE        hHandle,
                      unsigned char ucMachine,
                      unsigned char ucVideo,
                      unsigned char ucTAPversion,
                      unsigned int  uiByteCount);

// Outputs info on error status to console.
void TAP_CBM_OutputError(int Status);

#endif
