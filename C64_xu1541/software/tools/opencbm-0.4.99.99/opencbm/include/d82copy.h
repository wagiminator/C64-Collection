/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2001 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Copyright 2011-2011 Thomas Winkler <t(dot)winkler(at)tirol(dot)com>
*/

#ifndef D82COPY_H
#define D82COPY_H

//#define LIBD82COPY_DEBUG    /* enable state logging and debugging */

 
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




#ifdef __cplusplus
extern "C" {
#endif

/*
 *  known BAM modes
 */
typedef enum
{
    bm_ignore = 0,      /* all sectors                    */
    bm_allocated = 1,   /* allocated sectors              */
    bm_save = 2         /* allocated sectors + BAM track  */
} d82copy_bam_mode;

typedef enum
{
    em_always,
    em_on_error,
    em_never
} d82copy_error_mode;

typedef enum
{
    bs_invalid = 0,
    bs_dont_copy = 1,
    bs_must_copy = 2,
    bs_error = 3,
    bs_copied = 4
} d82copy_bam_status;

typedef struct
{
    int warp;
    int retries;
    int interleave;
    int start_track;
    int end_track;
    int two_sided;
    int transfer_mode;
    enum cbm_device_type_e drive_type;
    d82copy_bam_mode bam_mode;
    d82copy_error_mode error_mode;
} d82copy_settings;

typedef struct
{
    int track;
    int sector;
    int read_result;
    int write_result;
    int sectors_processed;
    int total_sectors;
    d82copy_settings *settings;
    char bam[MAX_TRACKS][MAX_SECTORS+1];
} d82copy_status;

typedef enum
{
    sev_fatal,
    sev_warning,
    sev_info,
    sev_debug
} d82copy_severity_e;

typedef void (*d82copy_message_cb)(int d82copy_severity_e, const char *format, ...);
typedef int (*d82copy_status_cb)(d82copy_status status);

#ifdef LIBD82COPY_DEBUG
/*
 * print out the state of internal counters that are used on read
 * and write transfers for debugging rare protocol races and hangups
 */
extern void printDebugLibD82Counters(d82copy_message_cb msg_cb);
#endif

/*
 * Build '\0'-terminated list of '\0'-terminated transfer mode names.
 * Memory should be free()'d after use.
 */
extern char *d82copy_get_transfer_modes();

/*
 * parse transfer mode name ("serial1", "s2", "parallel"...) abbreviations
 * are possible
 */
extern int d82copy_get_transfer_mode_index(const char *name);

/*
 * find out if "auto" transfer mode was specified. If yet, determine
 * the best transfer mode we can use.
 */
extern int d82copy_check_auto_transfer_mode(CBM_FILE cbm_fd,
                                            int auto_transfermode,
                                            int drive);

/*
 * returns malloc()'d pointer to default settings.
 * must be free()'d after use.
 */
extern d82copy_settings *d82copy_get_default_settings(void);

/*
 * return number of sectors on a given track, -1 if invalid
 */
extern int d82copy_sector_count(int two_sided, int track);

extern int d82copy_read_image(CBM_FILE cbm_fd,
                              d82copy_settings *settings,
                              int src_drive,
                              const char *dst_image,
                              d82copy_message_cb msg_cb,
                              d82copy_status_cb status_cb);

extern int d82copy_write_image(CBM_FILE cbm_fd,
                               d82copy_settings *settings,
                               const char *src_image,
                               int dst_drive,
                               d82copy_message_cb msg_cb,
                               d82copy_status_cb status_cb);

extern void d82copy_cleanup(void);


#ifdef __cplusplus
}
#endif

#endif  /* D82COPY_H */
