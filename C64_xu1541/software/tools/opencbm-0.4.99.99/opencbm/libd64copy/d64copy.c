/*
 *    This program is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public License
 *    as published by the Free Software Foundation; either version
 *    2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2001 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Modifications for cbm4win Copyright 2001-2004 Spiro Trikaliotis
*/

#include "d64copy_int.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "arch.h"


static const char d64_sector_map[MAX_TRACKS+1] =
{ 0,
  21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
  21, 21, 21, 21, 21, 21, 21, 19, 19, 19,
  19, 19, 19, 19, 18, 18, 18, 18, 18, 18,
  17, 17, 17, 17, 17,
  17, 17, 17, 17, 17, 17, 17
};

static const char d71_sector_map[D71_TRACKS+1] =
{ 0,
  21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
  21, 21, 21, 21, 21, 21, 21, 19, 19, 19,
  19, 19, 19, 19, 18, 18, 18, 18, 18, 18,
  17, 17, 17, 17, 17,
  21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
  21, 21, 21, 21, 21, 21, 21, 19, 19, 19,
  19, 19, 19, 19, 18, 18, 18, 18, 18, 18,
  17, 17, 17, 17, 17,
};

static const unsigned char warp_read_1541[] =
{
#include "warpread1541.inc"
};

static const unsigned char warp_write_1541[] =
{
#include "warpwrite1541.inc"
};

static const unsigned char warp_read_1571[] =
{
#include "warpread1571.inc"
};

static const unsigned char warp_write_1571[] =
{
#include "warpwrite1571.inc"
};

static const unsigned char turbo_read_1541[] =
{
#include "turboread1541.inc"
};

static const unsigned char turbo_write_1541[] =
{
#include "turbowrite1541.inc"
};

static const unsigned char turbo_read_1571[] =
{
#include "turboread1571.inc"
};

static const unsigned char turbo_write_1571[] =
{
#include "turbowrite1571.inc"
};

static const struct drive_prog
{
    int size;
    const unsigned char *prog;
} drive_progs[] =
{
    {sizeof(turbo_read_1541), turbo_read_1541},
    {sizeof(turbo_write_1541), turbo_write_1541},
    {sizeof(warp_read_1541), warp_read_1541},
    {sizeof(warp_write_1541), warp_write_1541},
    {sizeof(turbo_read_1571), turbo_read_1571},
    {sizeof(turbo_write_1571), turbo_write_1571},
    {sizeof(warp_read_1571), warp_read_1571},
    {sizeof(warp_write_1571), warp_write_1571}
};


static const int default_interleave[] = { -1, 17, 4, 13, 7, -1 };
static const int warp_write_interleave[] = { -1, 0, 6, 12, 4, -1 };


/*
 * Variables to make sure writing a block is an atomary process
 */
static int atom_mustcleanup = 0;
static const transfer_funcs *atom_dst;


#ifdef LIBD64COPY_DEBUG
    volatile signed int DebugLineNumber=-1, DebugBlockCount=-1,
                        DebugByteCount=-1,  DebugBitCount=-1;
    volatile char *     DebugFileName   = "";

    void printDebugLibD64Counters(d64copy_message_cb msg_cb)
    {
        msg_cb( sev_info, "file: %s"
                          "\n\tversion: " OPENCBM_VERSION ", built: " __DATE__ " " __TIME__
                          "\n\tline=%d, blocks=%d, bytes=%d, bits=%d\n",
                          DebugFileName, DebugLineNumber,
                          DebugBlockCount, DebugByteCount,
                          DebugBitCount);
    }
#endif

static int send_turbo(CBM_FILE fd, unsigned char drv, int write, int warp, int drv_type)
{
    const struct drive_prog *prog;

    prog = &drive_progs[drv_type * 4 + warp * 2 + write];

    SETSTATEDEBUG((void)0);
    return cbm_upload(fd, drv, 0x500, prog->prog, prog->size);
}

extern transfer_funcs d64copy_fs_transfer,
                      d64copy_std_transfer,
                      d64copy_pp_transfer,
                      d64copy_s1_transfer,
                      d64copy_s2_transfer;

static d64copy_message_cb message_cb;
static d64copy_status_cb status_cb;

int d64copy_sector_count(int two_sided, int track)
{
    if(two_sided)
    {
        if(track >= 1 && track <= D71_TRACKS)
        {
            return d71_sector_map[track];
        }
    }
    else
    {
        if(track >= 1 && track <= TOT_TRACKS)
        {
            return d64_sector_map[track];
        }
    }
    return -1;
}

d64copy_settings *d64copy_get_default_settings(void)
{
    d64copy_settings *settings;

    settings = malloc(sizeof(d64copy_settings));

    if(NULL != settings)
    {
        settings->warp        = -1;
        settings->retries     = 0;
        settings->bam_mode    = bm_ignore;
        settings->interleave  = -1; /* set later on */
        settings->start_track = 1;
        settings->end_track   = -1; /* set later on */
        settings->drive_type  = cbm_dt_unknown; /* auto detect later on */
        settings->two_sided   = 0;
        settings->error_mode  = em_on_error;
    }
    return settings;
}


static int start_turbo(CBM_FILE fd, unsigned char drive)
{
    SETSTATEDEBUG((void)0);
    return cbm_exec_command(fd, drive, "U4:", 3);
}


static int copy_disk(CBM_FILE fd_cbm, d64copy_settings *settings,
              const transfer_funcs *src, const void *src_arg,
              const transfer_funcs *dst, const void *dst_arg, unsigned char cbm_drive)
{
    unsigned char tr = 0;
    unsigned char se = 0;
    int st;
    int cnt  = 0;
    unsigned char scnt = 0;
    unsigned char errors;
    int retry_count;
    int resend_trackmap;
    int max_tracks;
    char trackmap[MAX_SECTORS+1];
    char buf[40];
    unsigned const char *bam_ptr;
    unsigned char bam[BLOCKSIZE];
    unsigned char bam2[BLOCKSIZE];
    unsigned char block[BLOCKSIZE];
    unsigned char gcr[GCRBUFSIZE];
    const transfer_funcs *cbm_transf = NULL;
    d64copy_status status;
    const char *sector_map;
    const char *type_str = "*unknown*";

    if(settings->two_sided)
    {
        max_tracks = D71_TRACKS;
    }
    else
    {
        max_tracks = TOT_TRACKS;
    }

    if(settings->interleave != -1 &&
           (settings->interleave < 1 || settings->interleave > 17))
    {
        message_cb(0,
                "invalid value (%d) for interleave", settings->interleave);
        return -1;
    }

    if(settings->start_track < 1 || settings->start_track > max_tracks)
    {
        message_cb(0,
                "invalid value (%d) for start track", settings->start_track);
        return -1;
    }

    if(settings->end_track != -1 && 
       (settings->end_track < settings->start_track ||
        settings->end_track > max_tracks))
    {
        message_cb(0,
                "invalid value (%d) for end track", settings->end_track);
        return -1;
    }

    if(settings->interleave == -1)
    {
        settings->interleave = (dst->is_cbm_drive && settings->warp) ?
            warp_write_interleave[settings->transfer_mode] :
            default_interleave[settings->transfer_mode];

        assert(settings->interleave >= 0);
    }


    if(settings->drive_type == cbm_dt_unknown )
    {
        message_cb( 2, "Trying to identify drive type" );
        if( cbm_identify( fd_cbm, cbm_drive, &settings->drive_type, NULL ) )
        {
            message_cb( 0, "could not identify device" );
        }

        switch( settings->drive_type )
        {
            case cbm_dt_cbm1541:
            case cbm_dt_cbm1570:
            case cbm_dt_cbm1571:
                /* fine */
                break;
            case cbm_dt_cbm1581:
                message_cb( 0, "1581 drives are not supported" );
                return -1;
            default:
                message_cb( 1, "Unknown drive, assuming 1541" );
                settings->drive_type = cbm_dt_cbm1541;
                break;
        }
    }

    sector_map = settings->two_sided ? d71_sector_map : d64_sector_map;

    SETSTATEDEBUG((void)0);
    cbm_exec_command(fd_cbm, cbm_drive, "I0:", 0);
    SETSTATEDEBUG((void)0);
    cnt = cbm_device_status(fd_cbm, cbm_drive, buf, sizeof(buf));
    SETSTATEDEBUG((void)0);

    switch( settings->drive_type )
    {
        case cbm_dt_cbm1541: type_str = "1541"; break;
        case cbm_dt_cbm1570: type_str = "1570"; break;
        case cbm_dt_cbm1571: type_str = "1571"; break;
        default: /* impossible */ break;
    }

    message_cb(cnt != 0 ? 0 : 2, "drive %02d (%s): %s",
               cbm_drive, type_str, buf );

    if(cnt)
    {
        return -1;
    }

    if(settings->two_sided)
    {
        if(settings->drive_type != cbm_dt_cbm1571)
        {
            message_cb(0, ".d71 transfer requires a 1571 drive");
            return -1;
        }
        if(settings->warp)
        {
            if(settings->warp>0)
                message_cb(1, "`-w' for .d71 transfer in warp mode ignored");
            settings->warp = 0;
        }
        SETSTATEDEBUG((void)0);
        cbm_exec_command(fd_cbm, cbm_drive, "U0>M1", 0);
    }

    SETSTATEDEBUG((void)0);
    cbm_transf = src->is_cbm_drive ? src : dst;

    if(settings->warp && (cbm_transf->read_gcr_block == NULL))
    {
        if(settings->warp>0)
            message_cb(1, "`-w' for this transfer mode ignored");
        settings->warp = 0;
    }

    settings->warp = settings->warp ? 1 : 0;

    if(cbm_transf->needs_turbo)
    {
        SETSTATEDEBUG((void)0);
        send_turbo(fd_cbm, cbm_drive, dst->is_cbm_drive, settings->warp,
                   settings->drive_type == cbm_dt_cbm1541 ? 0 : 1);
    }

    SETSTATEDEBUG((void)0);
    if(src->open_disk(fd_cbm, settings, src_arg, 0,
                      start_turbo, message_cb) == 0)
    {
        if(settings->end_track == -1)
        {
            settings->end_track = 
                settings->two_sided ? D71_TRACKS : STD_TRACKS;
        }
        SETSTATEDEBUG((void)0);
        if(dst->open_disk(fd_cbm, settings, dst_arg, 1,
                          start_turbo, message_cb) != 0)
        {
            message_cb(0, "can't open destination");
            return -1;
        }
    }
    else
    {
        message_cb(0, "can't open source");
        return -1;
    }

    memset(status.bam, bs_invalid, MAX_TRACKS * MAX_SECTORS);

    if(settings->bam_mode != bm_ignore)
    {
        if(settings->warp && src->is_cbm_drive)
        {
            memset(trackmap, bs_dont_copy, sector_map[18]);
            trackmap[0] = bs_must_copy;
            scnt = 1;
            SETSTATEDEBUG((void)0);
            src->send_track_map(18, trackmap, scnt);
            SETSTATEDEBUG(DebugBlockCount=0);
            st = src->read_gcr_block(&se, gcr);
            SETSTATEDEBUG(DebugBlockCount=-1);
            if(st == 0) st = gcr_decode(gcr, bam);
        }
        else
        {
            SETSTATEDEBUG(DebugBlockCount=0);
            st = src->read_block(18, 0, bam);
            if(settings->two_sided && (st == 0))
            {
                SETSTATEDEBUG(DebugBlockCount=1);
                st = src->read_block(53, 0, bam2);
            }
            SETSTATEDEBUG(DebugBlockCount=-1);
        }
        if(st)
        {
            message_cb(1, "failed to read BAM (%d)", st);
            settings->bam_mode = bm_ignore;
        }
    }
    SETSTATEDEBUG((void)0);

    memset(&status, 0, sizeof(status));

    /* setup BAM */
    for(tr = 1; tr <= max_tracks; tr++)
    {
        if(tr < settings->start_track || tr > settings->end_track)
        {
            memset(status.bam[tr-1], bs_dont_copy, sector_map[tr]);
        }
        else if(settings->bam_mode == bm_allocated ||
                (settings->bam_mode == bm_save && (tr % 35 != 18)))
        {
            for(se = 0; se < sector_map[tr]; se++)
            {
                if(settings->two_sided && tr > STD_TRACKS)
                {
                    bam_ptr = &bam2[3*(tr - STD_TRACKS - 1)];
                }
                else
                {
                    bam_ptr = &bam[4*tr + 1 + (tr > STD_TRACKS ? 48 : 0)];
                }
                if(bam_ptr[se/8]&(1<<(se&0x07)))
                {
                    status.bam[tr-1][se] = bs_dont_copy;
                }
                else
                {
                    status.bam[tr-1][se] = bs_must_copy;
                    status.total_sectors++;
                }
            }
        }
        else
        {
            status.total_sectors += sector_map[tr];
            memset(status.bam[tr-1], bs_must_copy, sector_map[tr]);
        }
    }

    status.settings = settings;

    status_cb(status);

    message_cb(2, "copying tracks %d-%d (%d sectors)",
            settings->start_track, settings->end_track, status.total_sectors);

    SETSTATEDEBUG(DebugBlockCount=0);
    for(tr = 1; tr <= max_tracks; tr++)
    {
        if(tr >= settings->start_track && tr <= settings->end_track)
        {
            scnt = sector_map[tr];
            memcpy(trackmap, status.bam[tr-1], scnt);
            if(settings->bam_mode != bm_ignore)
            {
                for(se = 0; se < sector_map[tr]; se++)
                {
                    if(trackmap[se] != bs_must_copy)
                    {
                        scnt--;
                    }
                }
            }

            retry_count = settings->retries;
            do
            {
                errors = resend_trackmap = 0;
                if(scnt && settings->warp && src->is_cbm_drive)
                {
                    SETSTATEDEBUG((void)0);
                    src->send_track_map(tr, trackmap, scnt);
                }
                else
                {
                    se = 0;
                }
                while(scnt && !resend_trackmap)
                {
                    if(settings->warp && src->is_cbm_drive)
                    {
                        SETSTATEDEBUG((void)0);
                        status.read_result = src->read_gcr_block(&se, gcr);
                        if(status.read_result == 0)
                        {
                            SETSTATEDEBUG((void)0);
                            status.read_result = gcr_decode(gcr, block);
                        }
                        else
                        {
                            /* mark all sectors not received so far */
                            /* ugly */
                            errors = 0;
                            for(scnt = 0; scnt < sector_map[tr]; scnt++)
                            {
                                if(NEED_SECTOR(trackmap[scnt]) && scnt != se)
                                {
                                    trackmap[scnt] = bs_error;
                                    errors++;
                                }
                            }
                            resend_trackmap = 1;
                        }
                    }
                    else
                    {
                        while(!NEED_SECTOR(trackmap[se]))
                        {
                            if(++se >= sector_map[tr]) se = 0;
                        }
                        SETSTATEDEBUG(DebugBlockCount++);
                        status.read_result = src->read_block(tr, se, block);
                    }

                    if(settings->warp && dst->is_cbm_drive)
                    {
                        SETSTATEDEBUG((void)0);
                        gcr_encode(block, gcr);
                        SETSTATEDEBUG(DebugBlockCount++);
                        status.write_result = 
                            dst->write_block(tr, se, gcr, GCRBUFSIZE-1,
                                             status.read_result);
                    }
                    else
                    {
                        SETSTATEDEBUG(DebugBlockCount++);
                        status.write_result = 
                            dst->write_block(tr, se, block, BLOCKSIZE,
                                             status.read_result);
                    }
                    SETSTATEDEBUG((void)0);

                    if(status.read_result)
                    {
                        /* read error */
                        trackmap[se] = bs_error;
                        errors++;
                        if(retry_count == 0)
                        {
                            status.sectors_processed++;
                            /* FIXME: shall we get rid of this? */
                            message_cb( 1, "read error: %02x/%02x: %d",
                                        tr, se, status.read_result );
                        }
                    }
                    else
                    {
                        /* successfull read */
                        if(status.write_result)
                        {
                            /* write error */
                            trackmap[se] = bs_error;
                            errors++;
                            if(retry_count == 0)
                            {
                                status.sectors_processed++;
                                /* FIXME: shall we get rid of this? */
                                message_cb(1, "write error: %02x/%02x: %d",
                                           tr, se, status.write_result);
                            }
                        }
                        else
                        {
                            /* successfull read and write, mark sector */
                            trackmap[se] = bs_copied;
                            cnt++;
                            status.sectors_processed++;
                        }
                    }
                    /* remaining sectors on this track */
                    if(!resend_trackmap)
                    {
                        scnt--;
                    }

                    status.track = tr;
                    status.sector= se;

                    status_cb(status);

                    if(dst->is_cbm_drive || !settings->warp)
                    {
                        se += (unsigned char) settings->interleave;
                        if(se >= sector_map[tr]) se -= sector_map[tr];
                    }
                }
                if(errors > 0 && settings->retries >= 0)
                {
                    retry_count--;
                    scnt = errors;
                }
            }
            while(retry_count >= 0 && errors > 0);
            if(errors)
            {
                message_cb(1, "giving up...");
            }
        }
        if(settings->two_sided)
        {
            if(tr <= STD_TRACKS)
            {
                if(tr + STD_TRACKS <= D71_TRACKS)
                {
                    tr += (STD_TRACKS - 1);
                }
            }
            else if(tr != D71_TRACKS)
            {
                tr -= STD_TRACKS;
            }
        }
    }
    SETSTATEDEBUG(DebugBlockCount=-1);

    dst->close_disk();
    SETSTATEDEBUG((void)0);
    src->close_disk();

    SETSTATEDEBUG((void)0);
    return cnt;
}


static struct _transfers
{
    const transfer_funcs *trf;
    const char *name, *abbrev;
}
transfers[] =
{
    { &d64copy_std_transfer, "auto", "a%" },
    { &d64copy_std_transfer, "original", "o%" },
    { &d64copy_s1_transfer, "serial1", "s1" },
    { &d64copy_s2_transfer, "serial2", "s2" },
    { &d64copy_pp_transfer, "parallel", "p%" },
    { NULL, NULL, NULL }
};

char *d64copy_get_transfer_modes()
{
    const struct _transfers *t;
    int size;
    char *buf;
    char *dst;

    size = 1; /* for terminating '\0' */
    for(t = transfers; t->trf; t++)
    {
        size += (strlen(t->name) + 1);
    }

    buf = malloc(size);

    if(buf)
    {
        dst = buf;
        for(t = transfers; t->trf; t++)
        {
            strcpy(dst, t->name);
            dst += (strlen(t->name) + 1);
        }
        *dst = '\0';
    }

    return buf;
}


int d64copy_get_transfer_mode_index(const char *name)
{
    const struct _transfers *t;
    int i;
    int abbrev_len;
    int tm_len;

    if(NULL == name)
    {
        /* default transfer mode */
        return 0;
    }

    tm_len = strlen(name);
    for(i = 0, t = transfers; t->trf; i++, t++)
    {
        if(arch_strcasecmp(name, t->name) == 0)
        {
            /* full match */
            return i;
        }
        if(t->abbrev[strlen(t->abbrev)-1] == '%')
        {
            abbrev_len = strlen(t->abbrev) - 1;
            if(abbrev_len <= tm_len && arch_strncasecmp(t->name, name, tm_len) == 0)
            {
                return i;
            }
        }
        else
        {
            if(strcmp(name, t->abbrev) == 0)
            {
                return i;
            }
        }
    }
    return -1;
}

int d64copy_check_auto_transfer_mode(CBM_FILE cbm_fd, int auto_transfermode, int drive)
{
    int transfermode = auto_transfermode;

    /* We assume auto is the first transfer mode */
    assert(strcmp(transfers[0].name, "auto") == 0);

    if (auto_transfermode == 0)
    {
        do {
            enum cbm_cable_type_e cable_type;
            unsigned char testdrive;

            /*
             * Test the cable
             */

            SETSTATEDEBUG((void)0);
            if (cbm_identify_xp1541(cbm_fd, (unsigned char)drive, NULL, &cable_type) == 0)
            {
                if (cable_type == cbm_ct_xp1541)
                {
                    /*
                     * We have a parallel cable, use that
                     */
                    SETSTATEDEBUG((void)0);
                    transfermode = d64copy_get_transfer_mode_index("parallel");
                    break;
                }
            }

            /*
             * We do not have a parallel cable. Check if we are the only drive
             * on the bus, so we can use serial2, at least.
             */

            for (testdrive = 4; testdrive < 31; ++testdrive)
            {
                enum cbm_device_type_e device_type;

                /* of course, the drive to be transfered to is present! */
                if (testdrive == drive)
                    continue;

                SETSTATEDEBUG((void)0);
                if (cbm_identify(cbm_fd, testdrive, &device_type, NULL) == 0)
                {
                    /*
                     * My bad, there is another drive -> only use serial1
                     */
                    SETSTATEDEBUG((void)0);
                    transfermode = d64copy_get_transfer_mode_index("serial1");
                    break;
                }
            }

            /*
             * If we reached here with transfermode 0, we are the only
             * drive, thus, use serial2.
             */
            SETSTATEDEBUG((void)0);
            if (transfermode == 0)
                transfermode = d64copy_get_transfer_mode_index("serial2");
            SETSTATEDEBUG((void)0);

        } while (0);
    }

    SETSTATEDEBUG((void)0);
    return transfermode;
}

int d64copy_read_image(CBM_FILE cbm_fd,
                       d64copy_settings *settings,
                       int src_drive,
                       const char *dst_image,
                       d64copy_message_cb msg_cb,
                       d64copy_status_cb stat_cb)
{
    const transfer_funcs *src;
    const transfer_funcs *dst;
    int ret;

    message_cb = msg_cb;
    status_cb = stat_cb;

    src = transfers[settings->transfer_mode].trf;
    dst = &d64copy_fs_transfer;

    atom_dst = dst;
    atom_mustcleanup = 1;

    SETSTATEDEBUG((void)0);
    ret = copy_disk(cbm_fd, settings,
            src, (void*)(ULONG_PTR)src_drive, dst, (void*)dst_image, (unsigned char) src_drive);

    atom_mustcleanup = 0;

    return ret;
}

int d64copy_write_image(CBM_FILE cbm_fd,
                        d64copy_settings *settings,
                        const char *src_image,
                        int dst_drive,
                        d64copy_message_cb msg_cb,
                        d64copy_status_cb stat_cb)
{
    const transfer_funcs *src;
    const transfer_funcs *dst;

    message_cb = msg_cb;
    status_cb = stat_cb;

    src = &d64copy_fs_transfer;
    dst = transfers[settings->transfer_mode].trf;

    SETSTATEDEBUG((void)0);
    return copy_disk(cbm_fd, settings,
            src, (void*)src_image, dst, (void*)(ULONG_PTR)dst_drive, (unsigned char) dst_drive);
}

void d64copy_cleanup(void)
{
    /* if we were interrupted writing to the fs, make sure to
     * write anything that has already been started
     */

    if (atom_mustcleanup)
    {
        atom_dst->close_disk();
        atom_mustcleanup = 0;
    }
}
