/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2004-2005 Spiro Trikaliotis
 *
 */

/*! **************************************************************
** \file sys/include/WINDOWS/config.h \n
** \author Spiro Trikaliotis \n
** \n
** \brief Definitions for the configuration of the cbm4win driver
**
****************************************************************/

/*! Only define if wait_for_listener() should use a DPC */
#define USE_DPC 1

/*! Only define if performance evaluation is to be compiled in */
#define PERFEVAL 1

/*! Only define if verbose performance evaluation is to be used. */
/* #define PERFEVAL_VERBOSE 1 /**/

/*! Only define if performance evaluation for parallel burst should be used */
/* #define PERFEVAL_PARBURST 1 /**/

/*! If this is defined, the QUEUE uses an implementation which
    changes fast to the working thread */
#define USE_FAST_START_THREAD 1

/*! If this is defined, we have TEST support for setting the
    BIDIR bit with cbm_iec_setrelease().*/
#define TEST_BIDIR 1
