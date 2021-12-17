/*
 *    This program is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public License
 *    as published by the Free Software Foundation; either version
 *    2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2001 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Modifications for openCBM Copyright 2011-2011 Thomas Winkler
*/

/* $Id: d82copy_int.h,v 1.7 2006-05-22 08:34:19 wmsr Exp $ */

#ifndef IMGCOPY_INT_H
#define IMGCOPY_INT_H

#include "opencbm.h"
#include "imgcopy.h"
#include "gcr.h"

#include "arch.h"


/*
8250, SFD-1001:
--------------------------------------------------
 1 logical side (2 physical sides)
 154 tracks (77 tracks/side x 2 sides)
   trks 1-39:    29 sectors/trk  (on physical side 1)
   trks 40-53:   27 sectors/trk   "     "       "  "
   trks 54-64:   25 sectors/trk   "     "       "  "
   trks 65-77:   23 sectors/trk   "     "       "  "
   trks 78-116:  29 sectors/trk  (on physical side 2)
   trks 117-130: 27 sectors/trk   "     "       "  "
   trks 131-141: 25 sectors/trk   "     "       "  "  
   trks 142-154: 23 sectors/trk   "     "       "  "
 Directory header at ?
 BAM starts at t38 s1
 BAM takes up 4 sectors
 Directory starts at t39 s1
 Directory takes up 29 sectors
 Free blocks: 4133


1581:
--------------------------------------------------
 1 logical side (2 physical sides)
 80 tracks 
   trks 1-80:    20 sectors/trk  (on physical side 1)
   trks 1-80:    20 sectors/trk  (on physical side 2)
 Directory header at 40/0
 BAM starts at  40/1
 BAM takes up 2 sectors
 Directory starts at 40/3
 Directory takes up 37sectors
 Free blocks: 

*/



/* standard .d64 track count */
#define D64_TRACKS   35
/* "standard" 40-track .d64 */
#define EXT_TRACKS   40
/* standard .d71 track count */
#define D71_TRACKS   70

/* standard .d80 track count */
#define D80_TRACKS   77
/* standard .d82 track count */
#define D82_TRACKS   154
/* absolute limit. may not work with all drives */
#define TOT_TRACKS   D82_TRACKS



#define D81_CAT_TRACK  	40							// catalog
#define D81_BAM_TRACK  	40							// BAM


#define D82_CAT_TRACK  	39							// catalog
#define D82_BAM_TRACK  	38							// BAM


#define D64_BLOCKS  683
#define D71_BLOCKS  (D64_BLOCKS*2)





#define NEED_SECTOR(b) ((((b)==bs_error)||((b)==bs_must_copy))?1:0)

#ifdef LIBD82COPY_DEBUG
    extern volatile signed int debugLibD82LineNumber, debugLibD82ByteCount, debugLibD82BitCount;
    extern volatile char *     debugLibD82FileName;
   #define SETSTATEDEBUG(_x)  \
        debugLibD82LineNumber=__LINE__; \
        debugLibD82FileName  =__FILE__; \
        (_x)
#else
   #define SETSTATEDEBUG(_x) 	
#endif



typedef int(*turbo_start)(CBM_FILE,unsigned char);

typedef struct {
    int  (*open_disk)(CBM_FILE,imgcopy_settings*,const void*,int,
                      turbo_start,imgcopy_message_cb);
    int  (*read_block)(unsigned char,unsigned char,unsigned char*);
    int  (*write_block)(unsigned char,unsigned char,const unsigned char*,int,int);
    void (*close_disk)(void);
    int  is_cbm_drive;
    int  needs_turbo;
    int  (*send_track_map)(imgcopy_settings*,unsigned char,const char*,unsigned char);
    int  (*read_gcr_block)(unsigned char*,unsigned char*);
} transfer_funcs;




typedef	unsigned char	uint8_t;
typedef	signed char		int8_t;

typedef	unsigned short	uint16_t	;
typedef	signed short		int16_t;




typedef struct 
{
	// ON TRACK 39/0 
	uint8_t			trkBAM;				// track of BAM
	uint8_t			secBAM;			// sector of BAM
	uint8_t			dos;				// $43 ("C") - DOS format version
	uint8_t			fil02;				// reserved
	uint16_t		fil03;					// unused
	uint8_t			diskName[16];		// disk name (in PETASCII, padded with $A0)
	uint8_t			fil05;				// $A0
	uint16_t		id;						// disk ID
	uint8_t			fil06;				// $A0
	uint16_t		ver;						// DOS version "2C"
	uint8_t			fil07[4];				// $A0
} st_d82_header;


typedef struct 
{
	uint8_t			trkNx;				// next track
	uint8_t			secNx;				// next sector 
	uint8_t			ver;					// DOS version "C"
	uint8_t			fil01;				// reserved
	uint8_t			trkLow;				// lowest track for this BAM		(1)
	uint8_t			trkHigh;				// highest track +1 for this BAM	(51)
	uint8_t			bam[50][5];			// 50 tracks	(1 .. 50)
} st_d82_BAM;


// DIRECTORY ENTRY

typedef struct 
{
	uint8_t			trkNx;				// next track
	uint8_t			secNx;				// next sector 
	uint8_t			fileType;			// 00=Scratched,80=DEL,81=SEQ,82=PRG,83=USR,84=REL
										// Bit 0-3:filetype,Bit 6:Locked flag (">"),
										// Bit 7:Closed flag  (produces  "*", or "splat")
	uint8_t			trkFil;				// file 1. blk track
	uint8_t			secFil;				// file 1. blk sector 
	uint8_t			fileName[16];		// file name (in PETASCII, padded with $A0)
	uint8_t			trkSide;				// track side blk	(REL files)
	uint8_t			secSide;			// sector side blk 	(REL files)
	uint8_t			recLen;				// record length 	(REL files)
	uint8_t			fil01[6];				// unused
	uint16_t		size;					// File size in sectors, low/high byte  order
} st_dirEntry;




typedef struct 
{
	// ON TRACK 40/0 
	uint8_t			trkCAT;				// track of Catalog
	uint8_t			secCAT;				// sector of Catalog
	uint8_t			dos;				// $44 ("D") - DOS format version - DOS 10
	uint8_t			fil02;				// reserved
	uint8_t			diskName[16];		// disk name (in PETASCII, padded with $A0)
	uint8_t			fil03;				// $A0
	uint8_t			fil04;				// $A0
	uint16_t			id;					// disk ID
	uint8_t			fil06;				// $A0
	uint16_t			ver;					// DOS type "3D"
} st_d81_header;


// BAM Contents, 40/1
typedef struct 
{
	uint8_t			trkNx;				// next track
	uint8_t			secNx;				// next sector 
	uint8_t			ver;					// DOS version "D"
	uint8_t			fil01;				// reserved		(0xBB)
	uint16_t			id;					// disk ID
	uint8_t			io;					// IO byte ??		(0xC0)
	uint8_t			autoboot;			// autoboot flag
	uint8_t			fil02[8];				// reserved
	uint8_t			bam[40][6];			// 40 tracks	(1 .. 40)
} st_d81_BAM;







#define DECLARE_TRANSFER_FUNCS(x,c,t) \
    transfer_funcs imgcopy_ ## x = {open_disk, \
                        read_block, \
                        write_block, \
                        close_disk, \
                        c, \
                        t, \
                        NULL, \
                        NULL}

#define DECLARE_TRANSFER_FUNCS_EX(x,c,t) \
    transfer_funcs imgcopy_ ## x = {open_disk, \
                        read_block, \
                        write_block, \
                        close_disk, \
                        c, \
                        t, \
                        send_track_map, \
                        read_gcr_block}

#endif
