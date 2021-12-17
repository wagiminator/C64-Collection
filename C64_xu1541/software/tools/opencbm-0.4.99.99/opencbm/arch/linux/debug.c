/*! ************************************************************** 
** \file arch/windows/debug.c \n
** \author Spiro Trikaliotis \n
** \version $Id: debug.c,v 1.4 2006-04-08 13:41:14 strik Exp $ \n
** \n
** \brief Debugging help functions and definitions
**
****************************************************************/

#ifndef DBG_KERNELMODE
    /*! Mark: We are in user-space (for debug.h) */
    #define DBG_USERMODE
#endif // #ifndef DBG_KERNELMODE

#include "debug.h"

#if DBG

#include <stdarg.h>
#include <stdio.h>



/*! Buffers where the debug string will be build up before it is
   send to the system (or into memory) */

unsigned char DbgBuffer[DBG_MAX_BUFFER][DBG_MAX_BUFFERLEN];

/*! Index into the DbgBuffer buffers, for each one one index */
int  DbgBufferPos[DBG_MAX_BUFFER];

/*! initialize debugging flags */
unsigned long DbgFlags = 0  // | 0x7FFFFFFF
//                       | DBGF_BREAK

//                       | DBGF_ENTER
//                       | DBGF_LEAVE
//                       | DBGF_LEAVE_FAILURE
//                       | DBGF_PARAM
#ifdef DBG_KERNELMODE
//                       | DBGF_IEC
//                       | DBGF_IRQ
                         | DBGF_ASSERTIRQL
//                       | DBGF_PORT
                         | DBGF_THREAD
//                       | DBGF_IRPPATH
//                       | DBGF_IRP
//                       | DBGF_DPC
                         | DBGF_DBGMEMBUF
                         | DBGF_DBGPRINT

#endif // #ifdef DBG_KERNELMODE
//                       | DBGF_PPORT
//                       | DBGF_SUCCESS
//                       | DBGF_ERROR
//                       | DBGF_WARNING
                         | DBGF_ASSERT
                         ;

#ifdef DBG_KERNELMODE

/*! In kernel mode, we have to handle SMP. Thus, we have many buffers.
    This variable contains the number of the next DebugBuffer to be checked if
    it is empty. */
LONG DebugBufferCounter;

/*! In kernel mode, we have to handle SMP. Thus, we have many buffers.
    This array contains a "usage bitmask". If the value is 0, the DebugBuffer
    is currently available, 1 means that it is busy and cannot be used. */
LONG DebugBufferUsed[DBG_MAX_BUFFER];

#endif // #ifdef DBG_KERNELMODE


/*! \brief Append something to the DebugBuffer

 This function appends a string to the DebugBuffer. It can be used
 with a format-string like printf().

 \param BufferNumber
   The number of the DebugBuffer in which the content is to be
   written.

 \param Format
   A printf()-style format specifier

 \param ...
   The variables which are used as parameters for the printf() style
   format specifier.
*/
void
DbgOutputIntoBuffer(unsigned long BufferNumber, const char * const Format, ...)
{
    va_list arg_ptr;

    // Get a pointer to the current position in the given DebugBuffer

    char *buffer = &DbgBuffer[BufferNumber][DbgBufferPos[BufferNumber]];

    va_start(arg_ptr, Format);

    // If there is some space left in the DebugBuffer, append the contents

    if ((DBG_MAX_BUFFERLEN - DbgBufferPos[BufferNumber] - 1) > 0) 
    {
        int n;
        
        // Output at most the number of bytes which are left in the DebugBuffer

        n = vsnprintf(buffer, DBG_MAX_BUFFERLEN - DbgBufferPos[BufferNumber] - 1,
            Format, arg_ptr);

        // Was the buffer too small? If yes, the number of bytes
        // inserted is the size of the remaining buffer, so, set
        // this value

        if (n<0) 
        {
            n = DBG_MAX_BUFFERLEN - DbgBufferPos[BufferNumber] - 1;
        }

        // advance the buffer into the DebugBuffer

        DbgBufferPos[BufferNumber] += n;

        // Make sure the string is null terminated

        DbgBuffer[BufferNumber][DbgBufferPos[BufferNumber]] = 0;
    }
    va_end(arg_ptr);
}

#endif // #if DBG
