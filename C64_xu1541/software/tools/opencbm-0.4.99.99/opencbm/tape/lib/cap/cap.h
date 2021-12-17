/*
 *  CBM 1530/1531 tape routines.
 *  Copyright 2012 Arnd Menge, arnd(at)jonnz(dot)de
*/

#ifndef __CAP_H_
#define __CAP_H_

#include <Windows.h>

// Status results from exported functions
#define CAP_Status_OK                              0
#define CAP_Status_OK_End_of_file                  1
#define CAP_Status_Error_Invalid_Handle           -1
#define CAP_Status_Error_Invalid_pointer          -2
#define CAP_Status_Error_Out_of_memory            -3
#define CAP_Status_Error_Creating_file            -4
#define CAP_Status_Error_File_not_found           -5
#define CAP_Status_Error_Closing_file             -6
#define CAP_Status_Error_File_not_open            -7
#define CAP_Status_Error_Seek_wrong_destination   -8
#define CAP_Status_Error_Seek_failed              -9
#define CAP_Status_Error_Reading_header           -10
#define CAP_Status_Error_Writing_header           -11
#define CAP_Status_Error_Reading_data             -12
#define CAP_Status_Error_Writing_data             -13
#define CAP_Status_Error_Wrong_signature          -14
#define CAP_Status_Error_Wrong_version            -15
#define CAP_Status_Error_Wrong_machine            -16
#define CAP_Status_Error_Wrong_video              -17
#define CAP_Status_Error_Wrong_precision          -18
#define CAP_Status_Error_Wrong_start_signal_edge  -19
#define CAP_Status_Error_Wrong_signal_format      -20
#define CAP_Status_Error_Wrong_signal_width       -21
#define CAP_Status_Error_Wrong_data_start_offset  -22

// Possible target machines for tape image
#define CAP_Machine_INVALID 0
#define CAP_Machine_C64     1
#define CAP_Machine_C16     2
#define CAP_Machine_VC20    3
#define CAP_Machine_Spec48K 4
#define CAP_Machine_CUSTOM  5

// Possible target video types for tape image
#define CAP_Video_INVALID   0
#define CAP_Video_PAL       1
#define CAP_Video_NTSC      2
#define CAP_Video_Spec48K   3
#define CAP_Video_CUSTOM    4

// Possible start signal edges for tape image
#define CAP_StartEdge_INVALID 0
#define CAP_StartEdge_Falling 1
#define CAP_StartEdge_Rising  2

// Possible signal formats for tape image
#define CAP_SignalFormat_INVALID  0
#define CAP_SignalFormat_Relative 1
#define CAP_SignalFormat_Absolute 2

// Possible signal widths for tape image
#define CAP_SignalWidth_INVALID 0
#define CAP_SignalWidth_40bit   1

// Default data start offset for tape image
#define CAP_Default_Data_Start_Offset 0xA0

// Create (overwrite) an image file for writing.
int CAP_CreateFile(HANDLE *hHandle, char *pcFilename);

// Open an existing image file for reading.
int CAP_OpenFile(HANDLE *hHandle, char *pcFilename);

// Close an image file.
int CAP_CloseFile(HANDLE *hHandle);

// Check if a file is already existing.
int CAP_isFilePresent(char *pcFilename);

// Return file size of image file (moves file pointer).
int CAP_GetFileSize(HANDLE hHandle, int *piFileSize);

// Seek to start of image file and read image header, extract & verify header contents, seek to start of image data.
int CAP_ReadHeader(HANDLE hHandle);

// Seek to start of file & write image header.
int CAP_WriteHeader(HANDLE hHandle);

// Write addon string after image header.
int CAP_WriteHeaderAddon(HANDLE hHandle, unsigned char *pucString, unsigned int uiStringLen);

// Read a signal from image, increment byte counter.
int CAP_ReadSignal(HANDLE hHandle, unsigned __int64 *pui64Signal, int *piCounter);

// Write a signal to image, increment counter for each written byte.
int CAP_WriteSignal(HANDLE hHandle, unsigned __int64 ui64Signal, int *piCounter);

// Verify header contents (Signature, Version, Precision, Machine, Video, StartEdge, SignalFormat, SignalWidth, StartOfs).
int CAP_isValidHeader(HANDLE hHandle);

// Return timestamp precision from header.
int CAP_GetHeader_Precision(HANDLE hHandle, unsigned int *puiPrecision);

// Set timestamp precision in header.
int CAP_SetHeader_Precision(HANDLE hHandle, unsigned int uiPrecision);

// Return target machine type from header.
int CAP_GetHeader_Machine(HANDLE hHandle, unsigned char *pucMachine);

// Set target machine type in header.
int CAP_SetHeader_Machine(HANDLE hHandle, unsigned char ucMachine);

// Return target video type from header.
int CAP_GetHeader_Video(HANDLE hHandle, unsigned char *pucVideo);

// Set target video type in header.
int CAP_SetHeader_Video(HANDLE hHandle, unsigned char ucVideo);

// Return start signal edge from header.
int CAP_GetHeader_StartEdge(HANDLE hHandle, unsigned char *pucStartEdge);

// Set start signal edge in header.
int CAP_SetHeader_StartEdge(HANDLE hHandle, unsigned char ucStartEdge);

// Set signal format in header.
int CAP_GetHeader_SignalFormat(HANDLE hHandle, unsigned char *pucSignalFormat);

// Return signal format from header.
int CAP_SetHeader_SignalFormat(HANDLE hHandle, unsigned char ucSignalFormat);

// Set signal width in header.
int CAP_GetHeader_SignalWidth(HANDLE hHandle, unsigned int *puiSignalWidth);

// Return signal width from header.
int CAP_SetHeader_SignalWidth(HANDLE hHandle, unsigned int uiSignalWidth);

// Set data start offset in header.
int CAP_SetHeader_StartOffset(HANDLE hHandle, unsigned int uiStartOffset);

// Get all header entries at once.
int CAP_GetHeader(HANDLE        hHandle,
                  unsigned int  *puiPrecision,
                  unsigned char *pucMachine,
                  unsigned char *pucVideo,
                  unsigned char *pucStartEdge,
                  unsigned char *pucSignalFormat,
                  unsigned int  *puiSignalWidth,
                  unsigned int  *puiStartOffset);

// Set all header entries at once.
int CAP_SetHeader(HANDLE        hHandle,
                  unsigned int  uiPrecision,
                  unsigned char ucMachine,
                  unsigned char ucVideo,
                  unsigned char ucStartEdge,
                  unsigned char ucSignalFormat,
                  unsigned int  uiSignalWidth,
                  unsigned int  uiStartOffset);

// Outputs info on error status to console.
void CAP_OutputError(int Status);

#endif
