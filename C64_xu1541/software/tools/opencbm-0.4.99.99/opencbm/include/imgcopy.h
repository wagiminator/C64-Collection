/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2001 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Copyright 2011-2011 Thomas Winkler <t(dot)winkler(at)tirol(dot)com>
*/

/* $Id: imgcopy.h,v 1.8 2011-04-03 diddl Exp $ */

#ifndef IMGCOPY_H
#define IMGCOPY_H

//#define LIBIMGCOPY_DEBUG    /* enable state logging and debugging */

 
/* standard .d80 track count */
#define D80_TRACKS   77
/* standard .d82 track count */
#define D81_TRACKS   80
/* standard .d82 track count */
#define D82_TRACKS   154
/* absolute limit. may not work with all drives */
#define TOT_TRACKS   D82_TRACKS


#define D80_STD_BLOCKS   ((39 * 29) + (14 * 27) + (11 * 25) + (13 * 23))

#define D80_BLOCKS  (D80_STD_BLOCKS)
#define D82_BLOCKS  (D80_STD_BLOCKS *2)

#define D81_BLOCKS  (40 * D81_TRACKS)

 
#define D80_MAX_SECTORS  29
#define D81_MAX_SECTORS  40


#define MAX_SECTORS		D81_MAX_SECTORS
#define MAX_TRACKS		D82_TRACKS




#ifdef __cplusplus
extern "C" {
#endif

/*
 *  known BAM modes
 */
typedef enum
{
	bm_ignore = 0,      		/* all sectors                    			*/
	bm_allocated = 1,   		/* allocated sectors              		*/
	bm_save = 2         		/* allocated sectors + BAM track  	*/
} imgcopy_bam_mode;


//
// supported types of image file
//	
typedef enum
{
	cbm_it_unknown = -1,    
	D64 = 0,      				/* all D64 types                			*/
	D71 = 1,   				/* D71				             		*/
	D80 = 2,   				/* D80				             		*/
	D81 = 3,   				/* D81				             		*/
	D82 = 4   				/* D82				             		*/
} imgcopy_image_type;


typedef enum
{
	em_always,
	em_on_error,
	em_never
} imgcopy_error_mode;

typedef enum
{
	bs_invalid = 0,
	bs_dont_copy = 1,
	bs_must_copy = 2,
	bs_error = 3,
	bs_copied = 4
} imgcopy_bam_status;

typedef struct
{
	int warp;
	int retries;
	int interleave;
	int start_track;
	int end_track;
	int two_sided;
	int transfer_mode;
	int cat_track;
	int bam_track;
	int max_tracks;
	int block_count;
	imgcopy_image_type image_type;								// selected imagetype cause image filename extension
	imgcopy_image_type image_type_std;							// standard imagetype for drive type
	enum cbm_device_type_e drive_type;
	imgcopy_bam_mode bam_mode;
	imgcopy_error_mode error_mode;
} imgcopy_settings;

typedef struct
{
    int track;
    int sector;
    int read_result;
    int write_result;
    int sectors_processed;
    int total_sectors;
    imgcopy_settings *settings;
    char bam[MAX_TRACKS+1][MAX_SECTORS+1];
} imgcopy_status;

typedef enum
{
    sev_fatal,
    sev_warning,
    sev_info,
    sev_debug
} imgcopy_severity_e;

typedef void (*imgcopy_message_cb)(int imgcopy_severity_e, const char *format, ...);
typedef int (*imgcopy_status_cb)(imgcopy_status status);



// Prototypes





#ifdef LIBIMGCOPY_DEBUG
/*
 * print out the state of internal counters that are used on read
 * and write transfers for debugging rare protocol races and hangups
 */
extern void printDebugLibImgCounters(imgcopy_message_cb msg_cb);
#endif

/*
 * Build '\0'-terminated list of '\0'-terminated transfer mode names.
 * Memory should be free()'d after use.
 */
extern char * imgcopy_get_transfer_modes(void);

/*
 * parse transfer mode name ("serial1", "s2", "parallel"...) abbreviations
 * are possible
 */
extern int imgcopy_get_transfer_mode_index(const char *name);

/*
 * find out if "auto" transfer mode was specified. If yet, determine
 * the best transfer mode we can use.
 */
extern int imgcopy_check_auto_transfer_mode(CBM_FILE cbm_fd,
                                            int auto_transfermode,
                                            int drive);

/*
 * returns malloc()'d pointer to default settings.
 * must be free()'d after use.
 */
extern imgcopy_settings *imgcopy_get_default_settings(void);

/*
 * return number of sectors on a given track, -1 if invalid
 */
extern int imgcopy_sector_count(imgcopy_settings *, int track);

extern int imgcopy_read_image(CBM_FILE cbm_fd,
                              imgcopy_settings *settings,
                              int src_drive,
                              const char *dst_image,
                              imgcopy_message_cb msg_cb,
                              imgcopy_status_cb status_cb);

extern int imgcopy_write_image(CBM_FILE cbm_fd,
                               imgcopy_settings *settings,
                               const char *src_image,
                               int dst_drive,
                               imgcopy_message_cb msg_cb,
                               imgcopy_status_cb status_cb);

extern void imgcopy_cleanup(void);


#ifdef __cplusplus
}
#endif

#endif  /* IMGCOPY_H */
