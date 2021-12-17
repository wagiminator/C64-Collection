/*
 *  CBM 1530/1531 tape routines.
 *  Copyright 2012 Arnd Menge, arnd(at)jonnz(dot)de
*/

#ifndef __CAP2SPEC48KTAP_H_
#define __CAP2SPEC48KTAP_H_

#include <Windows.h>

// Convert CAP to Spectrum48K TAP format. *EXPERIMENTAL*
__int32 CAP2SPEC48KTAP(HANDLE hCAP, FILE *TapFile);

#endif
