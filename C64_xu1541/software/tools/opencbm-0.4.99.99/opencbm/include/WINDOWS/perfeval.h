/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2005-2007 Spiro Trikaliotis
 */

/*! **************************************************************
** \file include/WINDOWS/perfeval.h \n
** \author Spiro Trikaliotis \n
** \n
** \brief Functions and macros for performance evaluation purposes
**
****************************************************************/

#ifndef PERFEVAL_H
#define PERFEVAL_H

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef _CPLUSPLUS */

#ifdef DBG_USERMODE
        typedef HANDLE PETHREAD;
#endif

#include "debug.h"

#ifdef PERFEVAL

/*! One entry of the performance evaluation. This is the
structure as it is written into the memory and/or the file */

typedef struct
PERFORMANCE_EVAL_ENTRY
{
    /*! the RDTSC timestamp */
    __int64 Timestamp;

    /*! The processor on which this event occurred. New in v2 */
    ULONG Processor;

    /*! The thread which is executing this event. New in v2 */
    PETHREAD PeThread;

    /*! The event which is logged */
    ULONG_PTR Event;

    /*! Additional data for the event */
    ULONG_PTR Data;

} PERFORMANCE_EVAL_ENTRY, *PPERFORMANCE_EVAL_ENTRY;

extern VOID PerfInit(VOID);
extern VOID PerfEvent(IN ULONG_PTR Event, IN ULONG_PTR Data);
extern VOID PerfSave(VOID);

/*! Call PerfInit() if performance evaluation is selected */
#define PERF_INIT() PerfInit()

/*! Call PerfEvent() if performance evaluation is selected */
#define PERF_EVENT(_Event_, _Data_) PerfEvent(_Event_, _Data_)

/*! Only define this non-empty if you want very verbose performance data */
#ifdef PERFEVAL_VERBOSE
  #define PERF_EVENT_VERBOSE(_Event_, _Data_) PERF_EVENT(_Event_, _Data_)
#else
  #define PERF_EVENT_VERBOSE(_Event_, _Data_)
#endif

#ifdef PERFEVAL_PARBURST
  #define PERF_EVENT_PARBURST(_Event_, _Data_) PERF_EVENT(_Event_, _Data_)
#else
  #define PERF_EVENT_PARBURST(_Event_, _Data_)
#endif

/*! Call PerfSave() if performance evaluation is selected */
#define PERF_SAVE() PerfSave()

#else /* #ifdef PERFEVAL */

/*! no performance evaluation, do nothing */
#define PERF_INIT()

/*! no performance evaluation, do nothing */
#define PERF_EVENT(_Event_, _Data_)

/*! no performance evaluation, do nothing */
#define PERF_SAVE()

#endif /* #ifdef PERFEVAL */

#ifdef __cplusplus
}
#endif /* #ifdef _CPLUSPLUS */

#endif // #ifndef PERFEVAL_H
