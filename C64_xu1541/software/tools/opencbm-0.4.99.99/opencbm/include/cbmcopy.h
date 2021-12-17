/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2001 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
*/

#ifndef CBMCOPY_H
#define CBMCOPY_H

#define LIBCBMCOPY_DEBUG

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int transfer_mode;
    enum cbm_device_type_e drive_type;
} cbmcopy_settings;

typedef enum
{
    sev_fatal,
    sev_warning,
    sev_info,
    sev_debug
} cbmcopy_severity_e;

typedef void (*cbmcopy_message_cb)(cbmcopy_severity_e sev, const char *format, ...);

typedef int (*cbmcopy_status_cb)(int blocks_processed);

#ifdef LIBCBMCOPY_DEBUG
/*
 * print out the state of internal counters that are used on read
 * and write transfers for debugging rare protocol races and hangups
 */
extern void printDebugCBMcopyCounters(cbmcopy_message_cb msg_cb);
#endif

/*
 * Build '\0'-terminated list of '\0'-terminated transfer mode names.
 * Memory should be free()'d after use.
 */
extern char *cbmcopy_get_transfer_modes();

/*
 * parse transfer mode name ("serial1", "s2", "parallel"...) abbreviations
 * are possible
 */
extern int cbmcopy_get_transfer_mode_index(const char *name);

/*
 * find out if "auto" transfer mode was specified. If yet, determine
 * the best transfer mode we can use.
 */
extern int cbmcopy_check_auto_transfer_mode(CBM_FILE cbm_fd,
                                            int auto_transfermode,
                                            int drive);

/*
 * returns malloc()'d pointer to default settings.
 * must be free()'d after use.
 */
extern cbmcopy_settings *cbmcopy_get_default_settings(void);

extern int cbmcopy_write_file(CBM_FILE cbm_fd,
                              cbmcopy_settings *settings,
                              int drive,
                              const char *cbmname,
                              int cbmname_size,
                              const unsigned char *filedata,
                              int filedata_size,
                              cbmcopy_message_cb msg_cb,
                              cbmcopy_status_cb status_cb);

extern int cbmcopy_read_file(CBM_FILE cbm_fd,
                             cbmcopy_settings *settings,
                             int drive,
                             const char *cbmname,
                             int cbmname_size,
                             unsigned char **filedata,
                             size_t *filedata_size,
                             cbmcopy_message_cb msg_cb,
                             cbmcopy_status_cb status_cb);

extern int cbmcopy_read_file_ts(CBM_FILE cbm_fd,
                                cbmcopy_settings *settings,
                                int drive,
                                int track, int sector,
                                unsigned char **filedata,
                                size_t *filedata_size,
                                cbmcopy_message_cb msg_cb,
                                cbmcopy_status_cb status_cb);

#ifdef __cplusplus
}
#endif

#endif  /* CBMCOPY_H */
