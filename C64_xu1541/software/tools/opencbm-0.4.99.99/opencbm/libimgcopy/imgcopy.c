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

#ifdef SAVE_RCSID 
static char *rcsid =
    "@(#) $Id: imgcopy.c,v 1.17 2011-04-23 12:24:31 wmsr Exp $";
#endif

#include "imgcopy_int.h"
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


//
// drive code 1581
//
static const unsigned char turbo_read_1581[] =
{
#include "turboread1581.inc"
};
static const unsigned char turbo_write_1581[] =
{
#include "turbowrite1581.inc"
};

//
// drive code 1541
//
static const unsigned char turbo_read_1541[] =
{
#include "turboread1541.inc"
};
static const unsigned char turbo_write_1541[] =
{
#include "turbowrite1541.inc"
};
static const unsigned char warp_read_1541[] =
{
#include "turboread1541.inc"
};
static const unsigned char warp_write_1541[] =
{
#include "turbowrite1541.inc"
};

//
// drive code 1571 / 1570
//
static const unsigned char turbo_read_1571[] =
{
#include "turboread1571.inc"
};
static const unsigned char turbo_write_1571[] =
{
#include "turbowrite1571.inc"
};
static const unsigned char warp_read_1571[] =
{
#include "turboread1571.inc"
};
static const unsigned char warp_write_1571[] =
{
#include "turbowrite1571.inc"
};





//
// debug function ...
//
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
			printf("%s\n", buf);
	}
}




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
	{sizeof(warp_write_1571), warp_write_1571},
	{sizeof(turbo_read_1581), turbo_read_1581},
	{sizeof(turbo_write_1581), turbo_write_1581},
	{0, NULL},												// no warp mode on 1581 devices
	{0, NULL},
	{0, NULL},
    //{sizeof(turbo_read_1541), turbo_read_1541},
};


static const int default_interleave[] = { -1, 22, -1 };
static const int warp_write_interleave[] = { -1, 0,-1 };


/*
 * Variables to make sure writing a block is an atomary process
 */
static int atom_mustcleanup = 0;
static const transfer_funcs *atom_dst;


#ifdef LIBIMGCOPY_DEBUG
    volatile signed int debugLibImgLineNumber=-1, debugLibImgBlockCount=-1,
                        debugLibImgByteCount=-1,  debugLibImgBitCount=-1;
    volatile char *     debugLibImgFileName   = "";

    void printDebugLibImgCounters(imgcopy_message_cb msg_cb)
    {
        msg_cb( sev_info, "file: %s"
                          "\n\tversion: " OPENCBM_VERSION ", built: " __DATE__ " " __TIME__
                          "\n\tline=%d, blocks=%d, bytes=%d, bits=%d\n",
                          debugLibImgFileName, debugLibImgLineNumber,
                          debugLibImgBlockCount, debugLibImgByteCount,
                          debugLibImgBitCount);
    }
#endif



//
// send drive code 
//
static int send_turbo(imgcopy_settings *settings, CBM_FILE fd, unsigned char drv, int write)
{
	//int warp, int drv_type :: settings->warp, settings->drive_type
	const struct drive_prog *prog;
	int warp, drv_type, idx;

	switch(settings->drive_type)
	{
	   case cbm_dt_cbm1541:
		drv_type = 0;
		break;

	   case cbm_dt_cbm1570:
	   case cbm_dt_cbm1571:
		drv_type = 1;
		break;

	   case cbm_dt_cbm1581:
		drv_type = 2;
		break;

	   case cbm_dt_cbm2040:
	   case cbm_dt_cbm2031:
	   case cbm_dt_cbm3040:
		   case cbm_dt_cbm4040:
	   case cbm_dt_cbm4031:
	   case cbm_dt_cbm8050:
	   case cbm_dt_cbm8250:
	   case cbm_dt_sfd1001:
	   case cbm_dt_unknown:
	   default:
		// drive type not allowed
		return -1;
	}

	warp = settings->warp ? 1 : 0;
	idx = drv_type * 4 + warp * 2 + write;
	printf("uploading drivecode %d\n", idx);
	prog = &drive_progs[idx];

	return cbm_upload(fd, drv, 0x500, prog->prog, prog->size) != prog->size;
}

extern transfer_funcs imgcopy_fs_transfer,
                      imgcopy_std_transfer;

static imgcopy_message_cb message_cb;
static imgcopy_status_cb status_cb;



//
// calculate the image file type
//
static int imgcopy_set_image_type(imgcopy_settings *settings, const char *filename)
{
	int i;

	if(settings->image_type == cbm_it_unknown && filename != NULL)
	{
		char 	*s;
		//message_cb(0, "parse file extension: %s", filename);
		if((s = strchr(filename, '.')) != NULL)
		{
			s++;
			//message_cb(0, "file extension: %s", s);
			if(arch_strcasecmp(s, "d64") == 0)
			{
				settings->image_type = D64;
				settings->two_sided = 0;
			}
			else if(arch_strcasecmp(s, "d71") == 0)
			{
				settings->image_type = D71;
				settings->two_sided = 1;
				message_cb(3, "imagetype D71 from file extension");
			}
			else if(arch_strcasecmp(s, "d80") == 0)
			{
				settings->image_type = D80;
				settings->two_sided = 0;
				message_cb(3, "imagetype D80 from file extension");
			}
			else if(arch_strcasecmp(s, "d81") == 0)
			{
				settings->image_type = D81;
				settings->two_sided = 0;
				message_cb(3, "imagetype D81 from file extension");
			}
			else if(arch_strcasecmp(s, "d82") == 0)
			{
				settings->image_type = D82;
				settings->two_sided = 1;
				message_cb(3, "imagetype D82 from file extension");
			}
		}
	}
	if(settings->image_type_std == cbm_it_unknown)
	{
		switch(settings->drive_type)
		{
		   case cbm_dt_cbm8050:
			settings->image_type_std = D80;
			message_cb(3, "imagetype D80 from drive type");
			break;

		   case cbm_dt_cbm8250:
		   case cbm_dt_sfd1001:
		   case cbm_dt_unknown:
			if(settings->two_sided)
			{
				settings->image_type_std = D82;
				message_cb(3, "imagetype D82 from drive type");
			}
			else
			{
				settings->image_type_std = D80;
				message_cb(3, "imagetype D80 from drive type");
			}
			break;

		   case cbm_dt_cbm1541:
		   case cbm_dt_cbm1570:
		   case cbm_dt_cbm2040:
		   case cbm_dt_cbm2031:
		   case cbm_dt_cbm3040:
    		   case cbm_dt_cbm4040:
		   case cbm_dt_cbm4031:
			settings->image_type_std = D64;
			break;

		   case cbm_dt_cbm1571:
			if(settings->two_sided)
			{
				settings->image_type_std = D71;
				message_cb(3, "imagetype D71 from drive type");
			}
			else
			{
				settings->image_type_std = D64;
			}
			break;
 
		   case cbm_dt_cbm1581:
			message_cb(3, "imagetype D81 from drive type");
			settings->image_type_std = D81;
			break;
		}
	}
	if(settings->image_type == cbm_it_unknown)
	{
		settings->image_type = settings->image_type_std;
		message_cb(2, "default imagetype from imagefile extension");
	}
	if(settings->image_type != settings->image_type_std)
	{
		if(settings->image_type_std == cbm_it_unknown)
			;// ok
		else if(settings->image_type_std == D71 && settings->image_type == D64)
			;// ok
		else if(settings->image_type_std == D82 && settings->image_type == D80)
			;// ok
		else
		{
			// invalid image type for this drive type
			message_cb(1, "invalid imagetype for this drive type");
			return -1;
		}
	}
	switch(settings->image_type)
	{
	   case D80:
		settings->max_tracks = D80_TRACKS;
		settings->cat_track = D82_CAT_TRACK;
		settings->bam_track = D82_BAM_TRACK;
		break;

	   case D82:
		settings->max_tracks = D82_TRACKS;
		settings->cat_track = D82_CAT_TRACK;
		settings->bam_track = D82_BAM_TRACK;
		break;

	   case D81:
		settings->max_tracks = D81_TRACKS;
		settings->cat_track = D81_CAT_TRACK;
		settings->bam_track = D81_BAM_TRACK;
		break;
	}

	settings->block_count = 0;
	for(i = 1; i <= settings->max_tracks; i++)
	{
		settings->block_count += imgcopy_sector_count(settings, i);
	}
	if(filename != NULL)		message_cb(3, "block count: %d", settings->block_count);
	return 0;
}



//
// calculate number of sectors for a track
//
int imgcopy_sector_count(imgcopy_settings *setting, int track)
{
	switch(setting->image_type)
	{
	   case D80:
		if(track >  D80_TRACKS)
			break;
	   case D82:
		if(track < 1 && track > D82_TRACKS)
			break;
		return d82_sector_map[track];

	   case D81:
		return 40;
	}
	return -1;
}



//
// check status of a block in BAM buffer
//
int ChkBAM(imgcopy_settings *settings, unsigned char *bam, int tr, int se)
{
	st_d81_BAM *bam81;
	st_d82_BAM *bam82;
	int offs, offs_tr, offs_se, rc, bam_byte;

	switch(settings->image_type)
	{
	   case D80:
	   case D82:
		offs = ((tr -1) /50 +1) * BLOCKSIZE;
		offs_tr = ((tr -1) %50) ;
		offs_se = se/8 +1;
		bam82 = (st_d82_BAM *)(bam +offs);
		bam_byte = bam82->bam[offs_tr][offs_se];
		rc = ((bam_byte & (1<< (se&0x07))) == 0);
		break;

	   case D81:
		offs = ((tr -1) /40 ) * BLOCKSIZE;
		offs_tr = ((tr -1) %40) ;
		offs_se = se/8 +1;
		bam81 = (st_d81_BAM *)(bam +offs);
		/*if(se == 0)
			DumpBlock((unsigned char *)bam81);*/
		bam_byte = bam81->bam[offs_tr][offs_se];
		rc = ((bam_byte & (1<< (se&0x07))) == 0);
		break;

	   default:
		rc = -1;
		break;
	}
	//printf("ChkBAM() :: rc=%d, tr=%d, se=%d, blk-offs=%d, tr-offs=%d, se-offs=%d, bambyte=%02x\n", rc, tr, se, offs, offs_tr, offs_se, bam_byte);
	return rc;
}



//
// set default settings
//
imgcopy_settings *imgcopy_get_default_settings(void)
{
	imgcopy_settings *settings;

	settings = malloc(sizeof(imgcopy_settings));

	if(NULL != settings)
	{
		settings->warp        = -1;
		settings->transfer_mode = 0;
		settings->retries     = 0;
		settings->bam_mode    = bm_ignore;
		settings->interleave  = -1; /* set later on */
		settings->start_track = 1;
		settings->end_track   = -1; /* set later on */
		settings->drive_type  = cbm_dt_unknown; /* auto detect later on */
		settings->image_type = cbm_it_unknown;
		settings->image_type_std = cbm_it_unknown;
		settings->two_sided   = -1; /* set later on */
		settings->error_mode  = em_on_error;
		settings->cat_track = 0;
		settings->bam_track = 0;
		settings->block_count = 0;
	}
	return settings;
}


//
// check transfer mode 
//
int imgcopy_check_transfer_mode(imgcopy_settings *settings)
{
	int transfermode = settings->transfer_mode;
	int mode_o = imgcopy_get_transfer_mode_index("original");
	int mode_s1 = imgcopy_get_transfer_mode_index("s1");
	int mode_s2 = imgcopy_get_transfer_mode_index("s2");
	int mode_s3 = imgcopy_get_transfer_mode_index("s3");
	int mode_p = imgcopy_get_transfer_mode_index("parallel");

	switch(settings->image_type)
	{
	   case D64:
	   case D71:
		// any transfermode allowed
		break;

	   case D81:
		// transfermode s1, s2, s3 allowed
		if(transfermode != mode_o && transfermode != mode_s2
				 && transfermode != mode_s1 &&  transfermode != mode_s3)
		{
			settings->transfer_mode = mode_o;
			//message_cb(1, "only transfermode 'original' allowed");
			return 1;
		}
		break;

	   case D80:
	   case D82:
	   default:
		// only transfermode 'o' allowed
		if(transfermode != mode_o)
		{
			settings->transfer_mode = mode_o;
			//message_cb(1, "only transfermode 'original' allowed");
			return 1;
		}
		break;
	}
	return 0;
}



//
// stgart drive code at $0503
//
static int start_turbo(CBM_FILE fd, unsigned char drive)
{
    SETSTATEDEBUG((void)0);
    return cbm_exec_command(fd, drive, "U4:", 3);
}




//
// read BAM of inserted disk
//
int ReadBAM_81(imgcopy_settings *settings, const transfer_funcs *src, unsigned char *buffer, int *bam_count)
{
	int cnt;
	int st;
	unsigned char track, sector;

	track =  D81_BAM_TRACK;
	sector = 1;
	*bam_count = 0;

	cnt = 0;
	while(1)
	{
		//message_cb(0, "reading BAM sector: %d / %d", track, sector);
		st = src->read_block(track, sector, buffer);
		if (st) break;

		//DumpBlock(buffer);

		if(++cnt >= 2)
			break;

		track = buffer[0];
		sector = buffer[1];
		if(track !=  D81_BAM_TRACK) 
		{
			message_cb(1, "BAM not at expected track: (%d)", track);
			st = 1;
			break;
		}

		buffer += BLOCKSIZE;
	}
	*bam_count = cnt;
	return st;
}




//
// read BAM of inserted disk
//
int ReadBAM_82(imgcopy_settings *settings, const transfer_funcs *src, unsigned char *buffer, int *bam_count)
{
	int cnt;
	int st;
	unsigned char track, sector;

	track =  D82_CAT_TRACK;
	sector = 0;
	*bam_count = 0;

	cnt = 0;
	while(1)
	{
		message_cb(2, "reading sector: %d / %d", track, sector);

		st = src->read_block(track, sector, buffer);
		if (st) break;

		//DumpBlock(buffer);

		if(++cnt >= 5)
			break;

		track = buffer[0];
		sector = buffer[1];
		if(track == D82_CAT_TRACK) 
		{
			if(cnt == 3) 
			{
				// 2 BAM blocks --- suppose a single sided (8050)
				if(settings->two_sided)
				{
					message_cb(1, "only 2 BAM blocks? suppose a D80 formatted disk ...");
					//settings->image_type = D80;
					//imgcopy_set_image_type(settings, NULL);
					st = 1;
				}
				break;
			}
			message_cb(0, "directory track isn't expected track for BAM: (track %d)", track);
			break;
		}
		if(track != D82_BAM_TRACK) 
		{
			message_cb(1, "BAM not at expected track: (%d)", track);
		}
		else if(cnt == 3 && !settings->two_sided)
		{
			message_cb(0, "more than 2 BAM blocks, suppose a D82 formatted disk!");
			break;
		}

		buffer += BLOCKSIZE;
	}
	*bam_count = cnt;
	return st;
}



//
// read BAM of inserted disk
//
int ReadBAM(imgcopy_settings *settings, const transfer_funcs *src, unsigned char *buffer, int *bam_count)
{
	message_cb(2, "reading BAM ...");
				
	switch(settings->image_type)
	{
	   case D80:
	   case D82:
		message_cb(2, "reading BAM of D82 ...");
		return ReadBAM_82(settings, src, buffer, bam_count);

	   case D81:
		return ReadBAM_81(settings, src, buffer, bam_count);
	}
	return -1;
}




static int copy_disk(CBM_FILE fd_cbm, imgcopy_settings *settings,
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
	char trackmap[MAX_SECTORS+1];
	char buf[40];
	//unsigned const char *bam_ptr;
	unsigned char bam[BLOCKSIZE *5];
	int bam_count;
	unsigned char block[BLOCKSIZE];
	//unsigned char gcr[GCRBUFSIZE];
	const transfer_funcs *cbm_transf = NULL;
	imgcopy_status status;
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
		    case cbm_dt_cbm4040:
		        message_cb( 0, "drive is not supported" );
		        return -1;
		    case cbm_dt_cbm1581:
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

	if(settings->two_sided == -1)
	{
		// set default
		switch( settings->drive_type )
		{
		   case cbm_dt_cbm8250:
		   case cbm_dt_sfd1001:
			settings->two_sided = 1;
			break;
		   case cbm_dt_cbm8050:
		   case cbm_dt_cbm1581:
		   default:
			settings->two_sided = 0;
			break;
		}
	}
	if(imgcopy_set_image_type(settings, NULL))
	{
		message_cb(0, "invalid imagetype for this drive type");
		return -1;
	}
	if(imgcopy_sector_count(settings, 1) < 0)
	{
		message_cb(0, "invalid drive or image type");
		return -1;
	}


	if(settings->interleave != -1 &&
	       (settings->interleave < 1 || settings->interleave > 24))
	{
		message_cb(0,
		        "invalid value (%d) for interleave", settings->interleave);
		return -1;
	}

	if(settings->start_track < 1 || settings->start_track > settings->max_tracks)
	{
	    message_cb(0,
	            "invalid value (%d) for start track. (MAXTRACKS=%d)", settings->start_track, settings->max_tracks);
	    return -1;
	}

	if(settings->end_track != -1 && 
	   (settings->end_track < settings->start_track ||
	    settings->end_track > settings->max_tracks))
	{
	    message_cb(0,
	            "invalid value (%d) for end track. (MAXTRACKS=%d)", settings->end_track, settings->max_tracks);
	    return -1;
	}

	if(settings->interleave == -1)
	{
		switch( settings->drive_type )
		{
		   case cbm_dt_cbm1581:
			settings->interleave = 1;				// always 1 cause track buffering
			break;
		   case cbm_dt_cbm8050:
		   case cbm_dt_cbm8250:
		   case cbm_dt_sfd1001:
		   default:
			settings->interleave = (dst->is_cbm_drive && settings->warp) ?
			    warp_write_interleave[settings->transfer_mode] :
			    default_interleave[settings->transfer_mode];
			break;
		}
		assert(settings->interleave >= 0);
	}

	SETSTATEDEBUG((void)0);
	cbm_exec_command(fd_cbm, cbm_drive, "I0:", 0);
	SETSTATEDEBUG((void)0);
	cnt = cbm_device_status(fd_cbm, cbm_drive, buf, sizeof(buf));
	SETSTATEDEBUG((void)0);

	switch( settings->drive_type )
	{
	    case cbm_dt_cbm8050: 	type_str = "CBM-8050"; break;
	    case cbm_dt_cbm8250: 	type_str = "CBM-8250"; break;
	    case cbm_dt_sfd1001:		type_str = "SFD-1001"; break;
	    case cbm_dt_cbm1581:	type_str = "CBM-1581"; break; 
	    default: 					type_str = "unknown"; break; 
	}

	if(cnt == 66)
	{
		// illegal track or sector :: DOS error
		if(settings->image_type == D80)
			cnt = 0;
		else
		{
			if(settings->image_type == D82)
			{
				//settings->image_type = D80;
				//imgcopy_set_image_type(settings, NULL);

				message_cb(1, "maybe a 8050 disk in a 8250 drive");
				cnt = 0;
			}
		}
	}
	if(cnt)
	{
		// DOS error
		message_cb(0, "drive %02d (%s): %s", cbm_drive, type_str, buf );
		return -1;
	}
	message_cb(2, "drive %02d (%s)", cbm_drive, type_str);


	if(settings->two_sided)
	{
		switch( settings->drive_type )
		{
		    case cbm_dt_cbm8250:
		    case cbm_dt_sfd1001:
			// ok
			break;
		    default:
			message_cb(0, "requires a two side drive");
			return -1;
		}
		SETSTATEDEBUG((void)0);
	}

	switch( settings->drive_type )
	{
	    case cbm_dt_cbm1541:
	    case cbm_dt_cbm1571:
		if(settings->warp && (cbm_transf->read_gcr_block == NULL))
		{
		    if(settings->warp>0)
		        message_cb(1, "`-w' for this transfer mode ignored");
		        settings->warp = 0;
		}
		break;

	    case cbm_dt_cbm1581:
	    case cbm_dt_cbm8050:
	    case cbm_dt_cbm8250:
	    case cbm_dt_sfd1001:
	    default:
	        if(settings->warp)
	        {
	            if(settings->warp>0)
	                message_cb(1, "drive type doesn't support warp mode");
                    settings->warp = 0;
	        }
		break;
	}

	//
	// Check if transfer mode is allowed
	//
 	if(imgcopy_check_transfer_mode(settings))
	{
	    message_cb(0, "transfer mode not allowed for this drive type");
	    return -1;
	}

	message_cb(2, "set transfer struc.");
	SETSTATEDEBUG((void)0);
	cbm_transf = src->is_cbm_drive ? src : dst;


	settings->warp = settings->warp ? 1 : 0;

	if(cbm_transf->needs_turbo)
	{
		int rc;
		message_cb(2, "sending turbo drive code ...");
		SETSTATEDEBUG((void)0);
//		send_turbo(fd_cbm, cbm_drive, dst->is_cbm_drive, settings->warp, settings->drive_type == cbm_dt_cbm1541 ? 0 : 1);
		if((rc=send_turbo(settings, fd_cbm, cbm_drive, dst->is_cbm_drive)) != 0)
		{
		    message_cb(0, "error while upload of drive code (rc=%d)", rc);
		    return -1;
		}
	}


	SETSTATEDEBUG((void)0);
	message_cb(2, "open source disk.");
	if(src->open_disk(fd_cbm, settings, src_arg, 0,
	                  start_turbo, message_cb) == 0)
	{
		if(settings->end_track == -1)
		{
			settings->end_track = settings->max_tracks;
		}
		SETSTATEDEBUG((void)0);
		message_cb(2, "open destination.");
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

	//message_cb(2, "set BAM buffer (%dx%d)", MAX_TRACKS, MAX_SECTORS);
	memset(status.bam, bs_invalid, MAX_TRACKS * MAX_SECTORS);

	if(settings->bam_mode != bm_ignore)
	{
		//message_cb(2, "reading BAM ...");
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
	//message_cb(3, "setup BAM (%d tracks)", settings->max_tracks);
	for(tr = 1; tr <= settings->max_tracks; tr++)
	{
		int sectorCount = imgcopy_sector_count(settings, tr);

		//message_cb(2, "track %d, sector %d", tr,  sectorCount);

		if(tr < settings->start_track || tr > settings->end_track)
		{
		    memset(status.bam[tr-1], bs_dont_copy, sectorCount);
		}
		else if(settings->bam_mode == bm_allocated ||
		        (settings->bam_mode == bm_save && (tr != settings->cat_track && tr != settings->bam_track )))
		{
			//message_cb(2, "offsets into BAM for track: tr=%d, se=%d", tr);
			/*char lbuf[100];
			gets(lbuf);*/
			for(se = 0; se < sectorCount; se++)
			{
				if(ChkBAM(settings, bam, tr, se))
				{
					//printf("copy track: %d, sector: %d\n", tr, se);
					status.bam[tr-1][se] = bs_must_copy;
					status.total_sectors++;
				}
				else
				{
					//printf("don't copy track: %d, sector: %d\n", tr, se);
					status.bam[tr-1][se] = bs_dont_copy;
				}
			}
		}
		else
		{
		    status.total_sectors += sectorCount;
		    memset(status.bam[tr-1], bs_must_copy, sectorCount);
		}
	}

	status.settings = settings;

	status_cb(status);

	message_cb(2, "copying tracks %d-%d (%d sectors)",
	        settings->start_track, settings->end_track, status.total_sectors);


	//
	// copy disk
	//
	SETSTATEDEBUG(debugLibImgBlockCount=0);
	for(tr = 1; tr <= settings->max_tracks; tr++)
	{
		unsigned char sectorCount = (unsigned char) imgcopy_sector_count(settings, tr);

		if(tr >= settings->start_track && tr <= settings->end_track)
		{
			memcpy(trackmap, status.bam[tr-1], sectorCount);
			retry_count = settings->retries;
			do
			{
				errors = resend_trackmap = 0;

				// calc count of blocks to copy
				scnt = sectorCount;
				if(settings->bam_mode != bm_ignore)
				{
				    for(se = 0; se < sectorCount; se++)
				    {
				        if(trackmap[se] != bs_must_copy)
				        {
				            scnt--;
				        }
				    }
				}
				//if(tr == 77)  printf("scnt=%d\n", scnt);

				if(scnt > 0 && settings->warp && src->is_cbm_drive)
				{
				    SETSTATEDEBUG((void)0);
				    src->send_track_map(settings, tr, trackmap, scnt);
				}
				else
				{
				    se = 0;
				}
				while(scnt > 0 && !resend_trackmap)
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
						int se_max = sectorCount;

						while(!NEED_SECTOR(trackmap[se]))
						{
						    if(++se >= sectorCount) se = 0;
						    if(se_max-- <= 0)	break;
						}
						if(se_max-- <= 0)	break;

						SETSTATEDEBUG(debugLibImgBlockCount++);
						status.read_result = src->read_block(tr, se, block);
					}

					/*if(settings->warp && dst->is_cbm_drive)
					{
					    SETSTATEDEBUG((void)0);
					    gcr_encode(block, gcr);
					    SETSTATEDEBUG(debugLibImgBlockCount++);
					    status.write_result = 
					        dst->write_block(tr, se, gcr, GCRBUFSIZE-1,
					                         status.read_result);
					}
					else  */
					{
					    SETSTATEDEBUG(debugLibImgBlockCount++);
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
						if(se >= sectorCount) se -= sectorCount;
					}
				}
				//if(tr == 77)  printf("after while\n");

				if(errors > 0)
				{
					retry_count--;
					scnt = errors;
				}
			} while(retry_count >= 0 && errors > 0);

			//if(tr == 77)  printf("after do\n");

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
		//message_cb(2, "track: %d, maxtrack=%d", tr, settings->max_tracks);
	}
	message_cb(2, "finished imagecopy.");

	SETSTATEDEBUG(debugLibImgBlockCount=-1);


	dst->close_disk();
	SETSTATEDEBUG((void)0);
	src->close_disk();

	SETSTATEDEBUG((void)0);
	return cnt;
}



extern transfer_funcs d64copy_fs_transfer,
                      imgcopy_std_transfer,
                      imgcopy_pp_transfer,
                      imgcopy_s1_transfer,
                      imgcopy_s2_transfer,
                      imgcopy_s3_transfer;



static struct _transfers
{
    const transfer_funcs *trf;
    const char *name, *abbrev;
}
transfers[] =
{
    { &imgcopy_std_transfer, "auto", "a%" },
    { &imgcopy_std_transfer, "original", "o%" },
    { &imgcopy_s1_transfer, "serial1", "s1" },
    { &imgcopy_s2_transfer, "serial2", "s2" },
    { &imgcopy_s3_transfer, "burst", "s3" },
    { &imgcopy_pp_transfer, "parallel", "p%" },
    { NULL, NULL, NULL }
};


//
// get name strings of all transfermodes
//
char *imgcopy_get_transfer_modes()
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


//
// get index of transfermode by givven name
//
int imgcopy_get_transfer_mode_index(const char *name)
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


//
// check auto transfer mode and replace default
//
int imgcopy_check_auto_transfer_mode(CBM_FILE cbm_fd, int auto_transfermode, int drive)
{
	int transfermode = auto_transfermode;

	assert(strcmp(transfers[0].name, "auto") == 0);

	if (auto_transfermode == 0)
	{
	        SETSTATEDEBUG((void)0);

	        if (transfermode == 0)
	            transfermode = imgcopy_get_transfer_mode_index("original");

	        SETSTATEDEBUG((void)0);
	}
	return transfermode;
}



//
// entry point :: read image file
//
int imgcopy_read_image(CBM_FILE cbm_fd,
                       imgcopy_settings *settings,
                       int src_drive,
                       const char *dst_image,
                       imgcopy_message_cb msg_cb,
                       imgcopy_status_cb stat_cb)
{
	const transfer_funcs *src;
	const transfer_funcs *dst;
	int ret;

	message_cb = msg_cb;
	status_cb = stat_cb;

	src = transfers[settings->transfer_mode].trf;
	dst = &imgcopy_fs_transfer;

	atom_dst = dst;
	atom_mustcleanup = 1;

	imgcopy_set_image_type(settings, dst_image);

	SETSTATEDEBUG((void)0);
	ret = copy_disk(cbm_fd, settings,
	        src, (void*)(ULONG_PTR)src_drive, dst, (void*)dst_image, (unsigned char) src_drive);

	atom_mustcleanup = 0;

	return ret;
}



//
// entry point :: write image file
//
int imgcopy_write_image(CBM_FILE cbm_fd,
                        imgcopy_settings *settings,
                        const char *src_image,
                        int dst_drive,
                        imgcopy_message_cb msg_cb,
                        imgcopy_status_cb stat_cb)
{
	const transfer_funcs *src;
	const transfer_funcs *dst;

	message_cb = msg_cb;
	status_cb = stat_cb;

	src = &imgcopy_fs_transfer;
	dst = transfers[settings->transfer_mode].trf;

	imgcopy_set_image_type(settings, src_image);

	SETSTATEDEBUG((void)0);
	return copy_disk(cbm_fd, settings,
	        src, (void*)src_image, dst, (void*)(ULONG_PTR)dst_drive, (unsigned char) dst_drive);
}

void imgcopy_cleanup(void)
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
