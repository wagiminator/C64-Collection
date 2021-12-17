/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2004, 2009 Spiro Trikaliotis
 *
 */

/*! ************************************************************** 
** \file libmisc/WINDOWS/perfeval.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Functions for performance evaluation purposes - USERMODE version
**
****************************************************************/

#include <windows.h>

#define DBG_USERMODE

#include "perfeval.h"

#include <stdlib.h>

#ifdef PERFEVAL

/*! define PERFEVAL_DEBUG if this file should be debugged */
#undef PERFEVAL_DEBUG 

/*! define PERFEVAL_WRITE_TO_MEMORY if the performance values should
 * not be written to a file, but to memory only. */
#undef PERFEVAL_WRITE_TO_MEMORY


#ifdef PERFEVAL_DEBUG

    #define DBG_PRINT_REG( _xxx ) DBG_PRINT( _xxx )
    #define DBG_PRINT_FILE( _xxx ) DBG_PRINT( _xxx )

#else /* #ifdef PERFEVAL_DEBUG */

    #define DBG_PRINT_REG( _xxx )
    #define DBG_PRINT_FILE( _xxx )

#endif /* #ifdef PERFEVAL_DEBUG */

/*! This macro defines the maximum number of events
 * to be sampled. */
#define MAX_PERFORMANCE_EVAL_ENTRIES 0xffff

/*! Pointer to the buffer of sampled events */
static PPERFORMANCE_EVAL_ENTRY PerformanceEvalEntries = NULL;

/*! The current entry to be written. This variable is
 * incremented *before* a value is written, thus, we 
 * start with -1. */
static ULONG CurrentPerformanceEvalEntry = -1;

/*! The frequency of this processor. We need this value
 * to be able to divide the processor ticks by this value,
 * giving the time in seconds. */
static ULONG ProcessorFrequency = -1;

/*! \brief Read the time-stamp counter

 This function reads the time stamp counter of pentium-class
 processors (and above).

 \return 
   This function returns the number of ticks since the
   processor has been started.

 \warning This function returns the TSC. While most processors
 run at a fixed frequency, this is not true for all processors.
 Especially laptop processors tend to use different running
 frequencies, depending on the load of the processor. For these,
 it is not possible to use the TSC for timing purposes, as the
 TSC does not increase linearly. Thus, handle with care!
*/
#pragma warning(push)
#pragma warning(disable:4035)

__forceinline __int64 RDTSC(void)
{
#ifdef USE_RDTSC
    __asm {
        // RDTSC
        _emit 0x0F
        _emit 0x31
    }
#else
    LARGE_INTEGER li;

    QueryPerformanceCounter(&li);

    return li.QuadPart;
#endif
}
#pragma warning(pop)

/*! \brief Initialize the performance sampling library

 This function initializes the performance sampling library.
 Essentially, this means
 \n 1. allocating memory for the entries
 \n 2. óbtaining the processor frequency
*/

VOID
PerfInit(VOID)
{
    FUNC_ENTER();

    DBG_ASSERT(PerformanceEvalEntries == NULL);
    DBG_ASSERT(CurrentPerformanceEvalEntry == -1);

    // Allocate memory for the entries

    PerformanceEvalEntries = (PPERFORMANCE_EVAL_ENTRY) malloc(
        MAX_PERFORMANCE_EVAL_ENTRIES * sizeof(*PerformanceEvalEntries));

    if (PerformanceEvalEntries)
    {
#ifdef USE_RDTSC

        UNICODE_STRING registryPath;
        NTSTATUS ntStatus;
        HANDLE handleRegistry;

        // Get the processor frequency from the registry. As Windows
        // already tried to calculate the frequency, do not try to
        // calculate it on my own, but rely on this.

        RtlInitUnicodeString(&registryPath, 
            L"\\REGISTRY\\MACHINE\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0");

        DBG_PRINT_REG((DBG_PREFIX "trying to open %wZ", &registryPath));
        ntStatus = cbm_registry_open_for_read(&handleRegistry, &registryPath);
        DBG_PRINT_REG((DBG_PREFIX "cbm_registry_open() returned %s", DebugNtStatus(ntStatus)));

        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = cbm_registry_read_ulong(handleRegistry, L"~MHz", &ProcessorFrequency);
            DBG_PRINT_REG((DBG_PREFIX "cbm_registry_read() returned %s, value = %u", 
                DebugNtStatus(ntStatus), ProcessorFrequency));

            ntStatus = cbm_registry_close(handleRegistry);
            DBG_PRINT_REG((DBG_PREFIX "cbm_registry_close() returned %s", DebugNtStatus(ntStatus)));
        }

#else
        LARGE_INTEGER li;

        QueryPerformanceCounter(&li);

        DBG_ASSERT(li.HighPart == 0);
        ProcessorFrequency = li.LowPart;

#endif
    }

    FUNC_LEAVE();
}

/*! \brief Write an event

 This function writes on event to the log.

 \param Event:
   A value telling which kind of event to log.

 \param Data:
   Additional data for that event.

 The meaning of Event and Data is totally up to the caller.
 The performance eval library does not try to give them
 any meaning at all.

 \todo Should we make sure to be running at elevated IRQL?
 Currently, we cannot be sure that the pointer into the buffer
 is not incremented more than just the number of processors
 bigger than the buffer length. If we allow to be scheduled
 (as we do now without elevating IRQL), every thread can increment
 this value. Anyhow, as we have an ULONG as pointer, it seems
 not very likely that we have a wrap around, isn't it? Thus,
 currently, I see no need to elevate IRQL.
*/

VOID
PerfEvent(IN ULONG_PTR Event, IN ULONG_PTR Data)
{
    ULONG currentEntry;

    FUNC_ENTER();

    // only try to write if we successfully allocated a buffer!

    if (PerformanceEvalEntries)
    {
        // first of all, increment the entry number of the event
        // This makes sure that we are multiprocessor-safe, as 
        // we only write to the log after we have successfully
        // incremented the value.

        currentEntry = InterlockedIncrement(&CurrentPerformanceEvalEntry);

        if (currentEntry < MAX_PERFORMANCE_EVAL_ENTRIES)
        {
            PerformanceEvalEntries[currentEntry].Timestamp = RDTSC();
            PerformanceEvalEntries[currentEntry].Processor = 0; // KeGetCurrentProcessorNumber();
            PerformanceEvalEntries[currentEntry].PeThread  = GetCurrentThread();
            PerformanceEvalEntries[currentEntry].Event = Event;
            PerformanceEvalEntries[currentEntry].Data = Data;
        }
        else
        {
            // If we incremented "out of the buffer", make sure
            // that we decrement the value back to where it was
            // before

            InterlockedDecrement(&CurrentPerformanceEvalEntry);
        }
    }

    FUNC_LEAVE();
}

/*! The file header of the performance entries in a file
*/
typedef 
struct PERFEVAL_FILE_HEADER
{
    /*! The version of this file */
    ULONG FileVersion;

    /*! The frequency of the processor which has sampled these
     values      */
    ULONG ProcessorFrequency;

    /*! The number of entries in this buffer */
    ULONG CountEntries;

} PERFEVAL_FILE_HEADER, *PPERFEVAL_FILE_HEADER;

/*! \brief Synchronize the performance entries

 This function synchronizes the performance entries. That is,
 it makes sure that "something is done" with them.

 If PEREVAL_WRITE_TO_MEMORY is defined, the entries are written
 to a file. If it is not defined, the values are output to the
 debug log.
*/

VOID
PerfSynchronize(VOID)
{
    FUNC_ENTER();

    // Make sure to only output the entries if there are some

    DBG_ASSERT(((LONG)CurrentPerformanceEvalEntry) >= 0);

    if (PerformanceEvalEntries)
    {

#ifdef PERFEVAL_WRITE_TO_MEMORY

        __int64 FirstTimestamp;
        __int64 LastTimestamp;
        ULONG i;

        FirstTimestamp = PerformanceEvalEntries[0].Timestamp;
        LastTimestamp = FirstTimestamp;
        
        for (i = 0; i < CurrentPerformanceEvalEntry; i++)
        {
            __int64 tempTimeAbs;
            __int64 tempTimeRel;
            ULONG usTime;
            ULONG msTime;
            ULONG sTime;

            tempTimeAbs = (PerformanceEvalEntries[i].Timestamp - FirstTimestamp) / ProcessorFrequency;
            tempTimeRel = (PerformanceEvalEntries[i].Timestamp - LastTimestamp) / ProcessorFrequency;

            LastTimestamp = PerformanceEvalEntries[i].Timestamp;

            usTime = (ULONG) (tempTimeAbs % 1000);
            msTime = (ULONG) ((tempTimeAbs / 1000) % 1000);
            sTime = (ULONG) ((tempTimeAbs / 1000000) % 1000);

            DBG_PRINT((DBG_PREFIX 
                "%6u - Time: %3u.%03u.%03u us (+%7I64u us) - Event = %08x, Data = %08x"
                " on processur %u in thread %p",
                i,
                sTime, msTime, usTime,
                tempTimeRel,
                PerformanceEvalEntries[i].Event,
                PerformanceEvalEntries[i].Data,
                PerformanceEvalEntries[i].Processor,
                PerformanceEvalEntries[i].PeThread));
        }

        // make sure we start storing the performance values 
        // at the beginning again

        InterlockedExchange(&CurrentPerformanceEvalEntry, -1);

#else /* #ifdef PERFEVAL_WRITE_TO_MEMORY */

        HANDLE fileHandle = CreateFile("time.perfeval", GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

        if (fileHandle != INVALID_HANDLE_VALUE)
        {
            PERFEVAL_FILE_HEADER fileHeader;
            DWORD byteWritten;

            fileHeader.FileVersion = 2
#ifndef _X86_
            | (1<<31);
#endif /* #ifndef _X86_ */
            ;

            fileHeader.ProcessorFrequency = ProcessorFrequency;
            fileHeader.CountEntries = CurrentPerformanceEvalEntry + 1;

            // the file has been opened; now, we can write to it

            WriteFile(fileHandle, &fileHeader, sizeof(fileHeader), &byteWritten, NULL);

            WriteFile(fileHandle, PerformanceEvalEntries, fileHeader.CountEntries * sizeof(*PerformanceEvalEntries), &byteWritten, NULL);

            CloseHandle(fileHandle);

            // make sure we start storing the performance values 
            // at the beginning again

            InterlockedExchange(&CurrentPerformanceEvalEntry, -1);
        }

#endif /* #ifdef PERFEVAL_WRITE_TO_MEMORY */

    }

    FUNC_LEAVE();
}

/*! \brief Stop sampling performance entries

 This function stops sampling perrformce entries. For this,
 it writes out everything that is already sampled, and then
 undoes the initialization.

 \warning 
 Make sure that his function does not compete with PerfEvent,
 as there is no protection at all against race conditions!
*/

#ifndef InterlockedExchangePointer
# define InterlockedExchangePointer(_x, _y) (void*) InterlockedExchange( (void*) (_x), (_y))
#endif /* #ifndef InterlockedExchangePointer */

VOID
PerfSave(VOID)
{
    FUNC_ENTER();

    if (PerformanceEvalEntries)
    {
        PVOID buffer;

        // write the event logs to where they should be written

        PerfSynchronize();

        // stop the event logging by writing the default values

        buffer = InterlockedExchangePointer(&PerformanceEvalEntries, 0);
        InterlockedExchange(&CurrentPerformanceEvalEntry, -1);

        // free the previously allocated buffer.

        free(buffer);
    }

    FUNC_LEAVE();
}

#endif /* #ifdef PERFEVAL */
