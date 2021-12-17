/*
 *  CBM 1530/1531 tape routines.
 *  Copyright 2012 Arnd Menge, arnd(at)jonnz(dot)de
*/

#ifndef __TAP_MISC_H_
#define __TAP_MISC_H_

#include <Windows.h>

// Macro to handle errors of called exported functions.
#define	Check_CAP_Error_TextRetM1(FuncRes) \
	{                                 \
		if (FuncRes != CAP_Status_OK) \
		{                             \
			CAP_OutputError(FuncRes); \
			return -1;                \
		}                             \
	}

// Macro to handle errors of called exported functions.
#define	Check_TAP_CBM_Error_TextRetM1(FuncRes) \
	{                                     \
		if (FuncRes != TAP_CBM_Status_OK) \
		{                                 \
			TAP_CBM_OutputError(FuncRes); \
			return -1;                    \
		}                                 \
	}

__int32 OutputError(__int32 Status);
__int32 OutputFuncError(__int32 Status);

#endif
