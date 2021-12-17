/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2011 Wolfgang Moser
 *  Copyright 2011 Spiro Trikaliotis
*/

#ifdef DEBUG_STATEDEBUG
    extern volatile int DebugLineNumber, DebugBlockCount,
                        DebugByteCount, DebugBitCount;
    extern volatile char *     DebugFileName;

#   define SETSTATEDEBUG(_x)  \
        DebugLineNumber=__LINE__; \
        DebugFileName  =__FILE__; \
        (_x)

    extern void DebugPrintDebugCounters(void);

#   define DEBUG_PRINTDEBUGCOUNTERS() \
        DebugPrintDebugCounters()

#else
#   define SETSTATEDEBUG(_x) do { } while (0)
#   define DEBUG_PRINTDEBUGCOUNTERS()
#endif


