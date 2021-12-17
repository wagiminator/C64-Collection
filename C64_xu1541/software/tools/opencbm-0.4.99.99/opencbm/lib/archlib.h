/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2005-2009 Spiro Trikaliotis
 */

#ifndef ARCHLIB_H
#define ARCHLIB_H

#include "opencbm.h"

#undef EXTERN

#include "opencbm-plugin.h"

#if defined WIN32

# if defined OPENCBM_PLUGIN
#  define EXTERN __declspec(dllexport) /*!< we are exporting the functions */
# else
#  define EXTERN __declspec(dllimport) /*!< we are importing the functions */
# endif

#else
# define EXTERN extern
#endif

EXTERN opencbm_plugin_get_driver_name_t            opencbm_plugin_get_driver_name;
EXTERN opencbm_plugin_driver_open_t                opencbm_plugin_driver_open;
EXTERN opencbm_plugin_driver_close_t               opencbm_plugin_driver_close;
EXTERN opencbm_plugin_lock_t                       opencbm_plugin_lock;
EXTERN opencbm_plugin_unlock_t                     opencbm_plugin_unlock;
EXTERN opencbm_plugin_raw_write_t                  opencbm_plugin_raw_write;
EXTERN opencbm_plugin_raw_read_t                   opencbm_plugin_raw_read;
EXTERN opencbm_plugin_listen_t                     opencbm_plugin_listen;
EXTERN opencbm_plugin_talk_t                       opencbm_plugin_talk;
EXTERN opencbm_plugin_open_t                       opencbm_plugin_open;
EXTERN opencbm_plugin_unlisten_t                   opencbm_plugin_unlisten;
EXTERN opencbm_plugin_untalk_t                     opencbm_plugin_untalk;
EXTERN opencbm_plugin_close_t                      opencbm_plugin_close;
EXTERN opencbm_plugin_unlisten_t                   opencbm_plugin_unlisten;
EXTERN opencbm_plugin_untalk_t                     opencbm_plugin_untalk;
EXTERN opencbm_plugin_get_eoi_t                    opencbm_plugin_get_eoi;
EXTERN opencbm_plugin_clear_eoi_t                  opencbm_plugin_clear_eoi;
EXTERN opencbm_plugin_reset_t                      opencbm_plugin_reset;
EXTERN opencbm_plugin_pp_read_t                    opencbm_plugin_pp_read;
EXTERN opencbm_plugin_pp_write_t                   opencbm_plugin_pp_write;
EXTERN opencbm_plugin_iec_poll_t                   opencbm_plugin_iec_poll;
EXTERN opencbm_plugin_iec_set_t                    opencbm_plugin_iec_set;
EXTERN opencbm_plugin_iec_release_t                opencbm_plugin_iec_release;
EXTERN opencbm_plugin_iec_setrelease_t             opencbm_plugin_iec_setrelease;
EXTERN opencbm_plugin_iec_wait_t                   opencbm_plugin_iec_wait;

EXTERN opencbm_plugin_parallel_burst_read_t        opencbm_plugin_parallel_burst_read;
EXTERN opencbm_plugin_parallel_burst_write_t       opencbm_plugin_parallel_burst_write;
EXTERN opencbm_plugin_parallel_burst_read_n_t      opencbm_plugin_parallel_burst_read_n;
EXTERN opencbm_plugin_parallel_burst_write_n_t     opencbm_plugin_parallel_burst_write_n;
EXTERN opencbm_plugin_parallel_burst_read_track_t  opencbm_plugin_parallel_burst_read_track;
EXTERN opencbm_plugin_parallel_burst_read_track_var_t opencbm_plugin_parallel_burst_read_track_var;
EXTERN opencbm_plugin_parallel_burst_write_track_t opencbm_plugin_parallel_burst_write_track;
EXTERN opencbm_plugin_parallel_burst_read_t        opencbm_plugin_srq_burst_read;
EXTERN opencbm_plugin_parallel_burst_write_t       opencbm_plugin_srq_burst_write;
EXTERN opencbm_plugin_parallel_burst_read_n_t      opencbm_plugin_srq_burst_read_n;
EXTERN opencbm_plugin_parallel_burst_write_n_t     opencbm_plugin_srq_burst_write_n;
EXTERN opencbm_plugin_parallel_burst_read_track_t  opencbm_plugin_srq_burst_read_track;
EXTERN opencbm_plugin_parallel_burst_write_track_t opencbm_plugin_srq_burst_write_track;

EXTERN opencbm_plugin_tap_prepare_capture_t        opencbm_plugin_tap_prepare_capture;
EXTERN opencbm_plugin_tap_prepare_write_t          opencbm_plugin_tap_prepare_write;
EXTERN opencbm_plugin_tap_get_sense_t              opencbm_plugin_tap_get_sense;
EXTERN opencbm_plugin_tap_wait_for_stop_sense_t    opencbm_plugin_tap_wait_for_stop_sense;
EXTERN opencbm_plugin_tap_wait_for_play_sense_t    opencbm_plugin_tap_wait_for_play_sense;
EXTERN opencbm_plugin_tap_motor_on_t               opencbm_plugin_tap_motor_on;
EXTERN opencbm_plugin_tap_motor_off_t              opencbm_plugin_tap_motor_off;
EXTERN opencbm_plugin_tap_start_capture_t          opencbm_plugin_tap_start_capture;
EXTERN opencbm_plugin_tap_start_write_t            opencbm_plugin_tap_start_write;
EXTERN opencbm_plugin_tap_get_ver_t                opencbm_plugin_tap_get_ver;
EXTERN opencbm_plugin_tap_download_config_t        opencbm_plugin_tap_download_config;
EXTERN opencbm_plugin_tap_upload_config_t          opencbm_plugin_tap_upload_config;
EXTERN opencbm_plugin_tap_break_t                  opencbm_plugin_tap_break;

EXTERN opencbm_plugin_s1_read_n_t                  opencbm_plugin_s1_read_n;
EXTERN opencbm_plugin_s1_write_n_t                 opencbm_plugin_s1_write_n;
EXTERN opencbm_plugin_s2_read_n_t                  opencbm_plugin_s2_read_n;
EXTERN opencbm_plugin_s2_write_n_t                 opencbm_plugin_s2_write_n;
EXTERN opencbm_plugin_pp_dc_read_n_t               opencbm_plugin_pp_dc_read_n;
EXTERN opencbm_plugin_pp_dc_write_n_t              opencbm_plugin_pp_dc_write_n;
EXTERN opencbm_plugin_pp_cc_read_n_t               opencbm_plugin_pp_cc_read_n;
EXTERN opencbm_plugin_pp_cc_write_n_t              opencbm_plugin_pp_cc_write_n;

EXTERN opencbm_plugin_iec_dbg_read_t               opencbm_plugin_iec_dbg_read;
EXTERN opencbm_plugin_iec_dbg_write_t              opencbm_plugin_iec_dbg_write;

EXTERN opencbm_plugin_init_t                       opencbm_plugin_init;
EXTERN opencbm_plugin_uninit_t                     opencbm_plugin_uninit;

#endif // #ifndef ARCHLIB_H
