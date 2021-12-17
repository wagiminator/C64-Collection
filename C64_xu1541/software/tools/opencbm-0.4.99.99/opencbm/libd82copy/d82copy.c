/*
 *    This program is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public License
 *    as published by the Free Software Foundation; either version
 *    2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2001 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Modifications for cbm4win Copyright 2001-2004 Spiro Trikaliotis
 *  Modifications for openCBM Copyright 2011-2011 Thomas Winkler
*/

#include "d82copy_int.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <assert.h> 

#include "arch.h"

/*
   trks 1-39:    29 sectors/trk  (on physical side 1)
   trks 40-53:   27 sectors/trk   "     "       "  "
   trks 54-64:   25 sectors/trk   "     "       "  "
   trks 65-77:   23 sectors/trk   "     "       "  "
*/

//#define STD_BLOCKS   ((39 * 29) + (14 * 27) + (11 * 25) + (13 * 23))


static const char d80_sector_map[D80_TRACKS+1] =
{ 0,							 //11
  29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,
  27,27,27,27,27,27,27,27,27,27,27,27,27,27,
  25,25,25,25,25,25,25,25,25,25,25,
  23,23,23,23,23,23,23,23,23,23,23,23,23
};

static const char d82_sector_map[D82_TRACKS+1] =
{ 0,
  29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,
  27,27,27,27,27,27,27,27,27,27,27,27,27,27,
  25,25,25,25,25,25,25,25,25,25,25,
  23,23,23,23,23,23,23,23,23,23,23,23,23,
  29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,
  27,27,27,27,27,27,27,27,27,27,27,27,27,27,
  25,25,25,25,25,25,25,25,25,25,25,
  23,23,23,23,23,23,23,23,23,23,23,23,23
};

/*
static const unsigned char warp_read_1541[] =
{
#include "warpread1541.inc"
};
*/ 


static const struct drive_prog
{
    int size;
    const unsigned char *prog;
} drive_progs[] =
{
    { 0, NULL }
    //{sizeof(turbo_read_1541), turbo_read_1541},
};


static const int default_interleave[] = { -1, 22, -1 };
static const int warp_write_interleave[] = { -1, 0,-1 };


/*
 * Variables to make sure writing a block is an atomary process
 */
static int atom_mustcleanup = 0;
static const transfer_funcs *atom_dst;


#ifdef LIBD82COPY_DEBUG
    volatile signed int debugLibD82LineNumber=-1, debugLibD82BlockCount=-1,
                        debugLibD82ByteCount=-1,  debugLibD82BitCount=-1;
    volatile char *     debugLibD82FileName   = "";

    void printDebugLibD82Counters(d82copy_message_cb msg_cb)
    {
        msg_cb( sev_info, "file: %s"
                          "\n\tversion: " OPENCBM_VERSION ", built: " __DATE__ " " __TIME__
                          "\n\tline=%d, blocks=%d, bytes=%d, bits=%d\n",
                          debugLibD82FileName, debugLibD82LineNumber,
                          debugLibD82BlockCount, debugLibD82ByteCount,
                          debugLibD82BitCount);
    }
#endif

static int send_turbo(CBM_FILE fd, unsigned char drv, int write, int warp, int drv_type)
{
    const struct drive_prog *prog;

    prog = &drive_progs[drv_type * 4 + warp * 2 + write];

    SETSTATEDEBUG((void)0);
    return cbm_upload(fd, drv, 0x500, prog->prog, prog->size);
}

extern transfer_funcs d82copy_fs_transfer,
                      d82copy_std_transfer;

static d82copy_message_cb message_cb;
static d82copy_status_cb status_cb;

int d82copy_sector_count(int two_sided, int track)
{
    if(two_sided)
    {
        if(track >= 1 && track <= D82_TRACKS)
        {
            return d82_sector_map[track];
        }
    }
    else
    {
        if(track >= 1 && track <=  D80_TRACKS)
        {
            return d80_sector_map[track];
        }
    }
    return -1;
}

d82copy_settings *d82copy_get_default_settings(void)
{
    d82copy_settings *settings;

    settings = malloc(sizeof(d82copy_settings));

    if(NULL != settings)
    {
        settings->warp        = -1;
        settings->retries     = 0;
        settings->bam_mode    = bm_ignore;
        settings->interleave  = -1; /* set later on */
        settings->start_track = 1;
        settings->end_track   = -1; /* set later on */
        settings->drive_type  = cbm_dt_unknown; /* auto detect later on */
        settings->two_sided   = -1; /* set later on */
        settings->error_mode  = em_on_error;
    }
    return settings;
}

static int start_turbo(CBM_FILE fd, unsigned char drive)
{
    SETSTATEDEBUG((void)0);
    return cbm_exec_command(fd, drive, "U4:", 3);
}

void DumpBlock(unsigned char *buffer)
{
	char buf[128];
	char buf2[6];
	int i;

	for(i =0; i<256; ++i)
	{
		if((i % 16) == 0)
			sprintf(buf, "%04x  ", i);

		sprintf(buf2, "%02x ", buffer[i]);
		strcat(buf, buf2);

		if((i % 16) == 15)
			message_cb(2, buf);
	}
}

int ReadBAM(d82copy_settings *settings, const transfer_funcs *src, unsigned char *buffer, int *bam_count)
{
	int cnt;
	int st;
	uint8_t track, sector;

	message_cb(2, "reading BAM ...");
				
	track = CAT_TRACK;
	sector = 0;
	*bam_count = 0;

	cnt = 0;
	while(1)
	{
		//message_cb(2, "reading sector: %d / %d", track, sector);

		st = src->read_block(track, sector, buffer);
		if (st) break;

		//DumpBlock(buffer);

		if(cnt++ >= 4)
			break;

		track = buffer[0];
		sector = buffer[1];
		if(track == CAT_TRACK) 
		{
			if(cnt == 3) 
			{
				// 2 BAM blocks --- suppose a single sided (8050)
				if(settings->two_sided)
				{
					message_cb(0, "double sided expected, but only 2 BAM blocks?!");
					st = 1;
				}
				break;
			}
			message_cb(0, "directory track isn't expected track for BAM: (track %d)", track);
			break;
		}
		if(track != BAM_TRACK) 
		{
			message_cb(1, "BAM not at expected track: (%d)", track);
		}
		else if(cnt == 3 && !settings->two_sided)
		{
			message_cb(1, "single sided expected, but more than 2 BAM blocks?!");
			st = 1;
			break;
		}

		buffer += BLOCKSIZE;
	}
	*bam_count = cnt;
	return st;
}

static int copy_disk(CBM_FILE fd_cbm, d82copy_settings *settings,
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
    //unsigned const char *bam_ptr;
    unsigned char bam[BLOCKSIZE *5];
    int bam_count;
    unsigned char block[BLOCKSIZE];
    //unsigned char gcr[GCRBUFSIZE];
    const transfer_funcs *cbm_transf = NULL;
    d82copy_status status;
    const char *sector_map;
    const char *type_str = "*unknown*";


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
            case cbm_dt_cbm1581:
            case cbm_dt_cbm4040:
                message_cb( 0, "drive is not supported" );
                return -1;
            case cbm_dt_cbm8050:
            case cbm_dt_cbm8250:
            case cbm_dt_sfd1001:
                /* fine */
                break;
            default:
                message_cb( 1, "Unknown drive, assuming 8250" );
                settings->drive_type = cbm_dt_cbm8250;
                break;
        }
    }

	if(settings->two_sided < 0)
	{
		switch( settings->drive_type )
		{
		  case cbm_dt_cbm8050:
			settings->two_sided = 0;
			break;
		    case cbm_dt_cbm8250:
		    case cbm_dt_sfd1001:
		    default:
			settings->two_sided = 1;
			break;
		}
	}


    if(settings->two_sided)
    {
        max_tracks = D82_TRACKS;
    }
    else
    {
        max_tracks = D80_TRACKS;
    }

    if(settings->interleave != -1 &&
           (settings->interleave < 1 || settings->interleave > 24))
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

    sector_map = settings->two_sided ? d82_sector_map : d80_sector_map;

    SETSTATEDEBUG((void)0);
    cbm_exec_command(fd_cbm, cbm_drive, "I0:", 0);
    SETSTATEDEBUG((void)0);
    cnt = cbm_device_status(fd_cbm, cbm_drive, buf, sizeof(buf));
    SETSTATEDEBUG((void)0);

    switch( settings->drive_type )
    {
            case cbm_dt_cbm8050: type_str = "CBM-8050"; break;
            case cbm_dt_cbm8250: type_str = "CBM-8250"; break;
            case cbm_dt_sfd1001: type_str = "SFD-1001"; break;
                break;
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
        if(settings->drive_type == cbm_dt_cbm8050)
        {
            message_cb(0, "requires a two side drive");
            return -1;
        }
        SETSTATEDEBUG((void)0);
    }

        if(settings->warp)
        {
            if(settings->warp>0)
                message_cb(1, "`-w' for transfer in warp mode ignored");
            settings->warp = 0;
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
                settings->two_sided ? D82_TRACKS : D80_TRACKS;
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
	st = ReadBAM(settings, src, bam, &bam_count);
	if(st)
	{
		message_cb(1, "failed to read BAM (%d), reading whole disk", st);
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
                (settings->bam_mode == bm_save && (tr != CAT_TRACK && tr != BAM_TRACK )))
        {
		int offs = ((tr -1) /50 +1) * BLOCKSIZE;
		offs += ((tr -1) %50) * 5 + 6;
		//bam_ptr = &bam[offs];
		//message_cb(2, "offsets into BAM for track: tr: %d:0-%d, offs:%d  ptr=%p", tr, sector_map[tr], offs, bam_ptr);

		for(se = 0; se < sector_map[tr]; se++)
		{
			int offs2 = offs + se/8 +1;
			if(bam[offs2]&(1<<(se&0x07)))
			{
				status.bam[tr-1][se] = bs_dont_copy;
			}
			else
			{
				//printf("track: %d, sector: %d, offs: %d, bam: %2x\n", tr, se, offs2, (int)(bam[offs2]));
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

    SETSTATEDEBUG(debugLibD82BlockCount=0);
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
                    /* if(settings->warp && src->is_cbm_drive)
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
                            // mark all sectors not received so far 
                            // ugly 
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
                    else */
                    {
                        while(!NEED_SECTOR(trackmap[se]))
                        {
                            if(++se >= sector_map[tr]) se = 0;
                        }
                        SETSTATEDEBUG(debugLibD82BlockCount++);
                        status.read_result = src->read_block(tr, se, block);
                    }

                    /*if(settings->warp && dst->is_cbm_drive)
                    {
                        SETSTATEDEBUG((void)0);
                        gcr_encode(block, gcr);
                        SETSTATEDEBUG(debugLibD82BlockCount++);
                        status.write_result = 
                            dst->write_block(tr, se, gcr, GCRBUFSIZE-1,
                                             status.read_result);
                    }
                    else  */
                    {
                        SETSTATEDEBUG(debugLibD82BlockCount++);
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
            if(tr <= D80_TRACKS)
            {
                if(tr + D80_TRACKS <= D82_TRACKS)
                {
                    tr += (D80_TRACKS -1);
                }
            }
            else if(tr != D82_TRACKS)
            {
                tr -= D80_TRACKS;
            }
        }
    }
    SETSTATEDEBUG(debugLibD82BlockCount=-1);

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
    { &d82copy_std_transfer, "auto", "a%" },
    { &d82copy_std_transfer, "original", "o%" },
    { NULL, NULL, NULL }
};

char *d82copy_get_transfer_modes()
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


int d82copy_get_transfer_mode_index(const char *name)
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


int d82copy_check_auto_transfer_mode(CBM_FILE cbm_fd, int auto_transfermode, int drive)
{
	int transfermode = auto_transfermode;

	assert(strcmp(transfers[0].name, "auto") == 0);

	if (auto_transfermode == 0)
	{
	        SETSTATEDEBUG((void)0);

	        if (transfermode == 0)
	            transfermode = d82copy_get_transfer_mode_index("original");

	        SETSTATEDEBUG((void)0);
	}
	return transfermode;
}


int d82copy_read_image(CBM_FILE cbm_fd,
                       d82copy_settings *settings,
                       int src_drive,
                       const char *dst_image,
                       d82copy_message_cb msg_cb,
                       d82copy_status_cb stat_cb)
{
    const transfer_funcs *src;
    const transfer_funcs *dst;
    int ret;

    message_cb = msg_cb;
    status_cb = stat_cb;

    src = transfers[settings->transfer_mode].trf;
    dst = &d82copy_fs_transfer;

    atom_dst = dst;
    atom_mustcleanup = 1;

    SETSTATEDEBUG((void)0);
    ret = copy_disk(cbm_fd, settings,
            src, (void*)(ULONG_PTR)src_drive, dst, (void*)dst_image, (unsigned char) src_drive);

    atom_mustcleanup = 0;

    return ret;
}

int d82copy_write_image(CBM_FILE cbm_fd,
                        d82copy_settings *settings,
                        const char *src_image,
                        int dst_drive,
                        d82copy_message_cb msg_cb,
                        d82copy_status_cb stat_cb)
{
    const transfer_funcs *src;
    const transfer_funcs *dst;

    message_cb = msg_cb;
    status_cb = stat_cb;

    src = &d82copy_fs_transfer;
    dst = transfers[settings->transfer_mode].trf;

    SETSTATEDEBUG((void)0);
    return copy_disk(cbm_fd, settings,
            src, (void*)src_image, dst, (void*)(ULONG_PTR)dst_drive, (unsigned char) dst_drive);
}

void d82copy_cleanup(void)
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
