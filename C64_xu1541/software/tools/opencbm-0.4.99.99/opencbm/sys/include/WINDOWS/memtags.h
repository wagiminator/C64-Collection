/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2004 Spiro Trikaliotis
 *
 */

/*! **************************************************************
** \file sys/include/WINDOWS/memtags.h \n
** \author Spiro Trikaliotis \n
** \n
** \brief Define tags for ExAllocatePoolWithTag()
**
****************************************************************/


/*! Make sure we *never* use untagged memory allocation */
#ifdef ExAllocatePool
    #undef ExAllocatePool
#endif

/*! memory tag for the device name */
#define MTAG_DEVNAME 'ndTS'    // STdn

/*! memory tag for the enumeration struct */
#define MTAG_SENUMERATE 'neTS' // STen

/*! memory tag for the PARALLEL_PORT_INFO struct */
#define MTAG_PPINFO 'ppTS'     // STpp

/*! memory tag for the ServiceKeyRegistryPath UNICODE string */
#define MTAG_SERVKEY 'ksTS'     // STsk

/*! memory tag for the perfeval functions */
#define MTAG_PERFEVAL 'epTS'     // STpe

/*! memory tag for the debug buffer */
#define MTAG_DBGBUFFER 'bdTS'    // STdb
