/*
 *    This program is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public License
 *    as published by the Free Software Foundation; either version
 *    2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2001 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Modifications for openCBM Copyright 2011-2011 Thomas Winkler
*/

#ifndef D82COPY_INT_H
#define D82COPY_INT_H

#include "opencbm.h"
#include "d82copy.h"
#include "gcr.h"

#include "arch.h"

#ifdef LIBD82COPY_DEBUG
# define DEBUG_STATEDEBUG
#endif
#include "statedebug.h"


/*
8250, SFD-1001 (?):
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
 BAM starts at t38 s?
 BAM takes up ? sectors
 Directory starts at t39 s1
 Directory takes up ? sectors
 Free blocks: 4133
*/


/* standard .d80 track count */
#define D80_TRACKS   77
/* standard .d82 track count */
#define D82_TRACKS   154
/* absolute limit. may not work with all drives */
#define TOT_TRACKS   D82_TRACKS



#define STD_BLOCKS   ((39 * 29) + (14 * 27) + (11 * 25) + (13 * 23))
#define D80_BLOCKS  (STD_BLOCKS)
#define D82_BLOCKS  (STD_BLOCKS *2)

#define MAX_SECTORS  29
#define MAX_TRACKS   D82_TRACKS

#define NEED_SECTOR(b) ((((b)==bs_error)||((b)==bs_must_copy))?1:0)


#define CAT_TRACK  		39							// catalog
#define BAM_TRACK  	38							// BAM



typedef int(*turbo_start)(CBM_FILE,unsigned char);

typedef struct {
    int  (*open_disk)(CBM_FILE,d82copy_settings*,const void*,int,
                      turbo_start,d82copy_message_cb);
    int  (*read_block)(unsigned char,unsigned char,unsigned char*);
    int  (*write_block)(unsigned char,unsigned char,const unsigned char*,int,int);
    void (*close_disk)(void);
    int  is_cbm_drive;
    int  needs_turbo;
    int  (*send_track_map)(unsigned char,const char*,unsigned char);
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
	uint8_t			bam[5*50];			// 50 tracks	(1 .. 50)
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






#define DECLARE_TRANSFER_FUNCS(x,c,t) \
    transfer_funcs d82copy_ ## x = {open_disk, \
                        read_block, \
                        write_block, \
                        close_disk, \
                        c, \
                        t, \
                        NULL, \
                        NULL}

#define DECLARE_TRANSFER_FUNCS_EX(x,c,t) \
    transfer_funcs d82copy_ ## x = {open_disk, \
                        read_block, \
                        write_block, \
                        close_disk, \
                        c, \
                        t, \
                        send_track_map, \
                        read_gcr_block}

#endif
