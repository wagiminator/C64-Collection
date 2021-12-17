/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2001 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Copyright 2004-2007 Spiro Trikaliotis
 *  Copyright 2011-2011 Thomas Winkler
*/
 
#ifdef SAVE_RCSID
static char *rcsid =
    "@(#) $Id: main.c,v 1.13 2011-04-11 17:51:38 strik Exp $";  
#endif
  
#include "opencbm.h" 
#include "imgcopy.h"     
   
#include "arch.h"    
#include "libmisc.h" 
   
#include <getopt.h>       
#include <stdarg.h>  
#include <stdio.h>      
#include <stdlib.h>      
#include <string.h>    

 
/* setable via command line */   
static imgcopy_severity_e verbosity = sev_warning;    
static int no_progress = 0; 
  
/* other globals */
static CBM_FILE fd_cbm;  


static int is_cbm(char *name) 
{  
    return((strcmp(name, "8" ) == 0) || (strcmp(name, "9" ) == 0) ||
           (strcmp(name, "10") == 0) || (strcmp(name, "11") == 0) );
}


static void help(void)
{
    printf(
"Usage: imgcopy [OPTION]... [SOURCE] [TARGET]\n"
"Copy .d82 and .d80 disk images to a CBM-8050 or compatible drive and vice versa\n"
"Copy .d81 disk images to a 1581 or compatible drive and vice versa\n"
"\n"
"The extension of Imagfile select Imagefiletype (.d64, .d71, .d80, .d81, .d82)\n"
"\n"
"Options:\n"
"  -h, --help               display this help and exit\n"
"  -V, --version            display version information and exit\n"
"  -@, --adapter=plugin:bus tell OpenCBM which backend plugin and bus to use\n"
"  -q, --quiet              quiet output\n"
"  -v, --verbose            control verbosity (repeatedly, up to 3 times)\n"
"  -n, --no-progress        do not display progress information\n"
"\n"
"  -s, --start-track=TRACK  set start track\n"
"  -e, --end-track=TRACK    set end track (start <= end <= 77 / 154)\n"
"\n"
"  -t, --transfer=TRANSFER  set transfermode; valid modes:\n" 
"                             auto (default)\n"
"                             original       (slowest)\n"
"                           'auto' tries to determine the best option.\n"
"\n"
"  -i, --interleave=VALUE   set interleave value; ignored when reading with\n"
"                           warp mode; default values are:\n"
"\n"
"                             original     22\n"
"\n"
"                           INTERLEAVE is ignored when reading with warp mode;\n"
"                           if data transfer is very slow, increasing this\n"
"                           value may help.\n"
"\n"
"  -w, --warp               enable warp mode; this is not possible if\n"
"                           TRANSFER is set to 'original'\n"
"                           This is the default if transfer is not 'original'.\n"
"\n"
"      --no-warp            disable warp mode; this is the default if\n"
"                           TRANSFER is set to 'original'.\n"
"\n"
"  -b, --bam-only           BAM-only copy; only allocated blocks are copied;\n"
"\n"
"  -B, --bam-save           save BAM-only copy; this is like the '-b' option\n"
"                           but copies always the entire directory track.\n"
"\n"
"  -d, --drive-type=TYPE    specify drive type:\n"
"                             1541, 1571, 1581\n"
"                             2031, 2040, 3040, 4031, 4040\n"
"                             8050, 8250, 1001\n"
"\n"
"  -r, --retry-count=COUNT  set retry count\n"
"\n"
"  -E, --error-map=WHEN     control whether the error map is appended.\n"
"                           possible values for WHEN are (abbreviations\n"
"                           available):\n"
"                             always\n"
"                             on_errors     (default)\n"
"                             never\n"
"\n"
"  -1, --one-sided          one-sided disk transfer (.d80) for CBM-8050\n"
"\n"
"  -2, --two-sided          two-sided disk transfer (.d82): Requires CBM-8250 or SFD-1001.\n"
"\n"
);
}

//
// print help screen
//
static void hint(char *s)
{
    fprintf(stderr, "Try '%s' --help for more information.\n", s);
}


//
// debug function
//
static void my_message_cb(int severity, const char *format, ...)
{
    va_list args;

    static const char *severities[4] =
    {
        "Fatal",
        "Warning",
        "Info",
        "Debug"
    };

    if(verbosity >= severity)
    {
        fprintf(stderr, "[%s] ", severities[severity]);
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        fprintf(stderr, "\n");
    }
}

//
// print status line while copy
//
static int my_status_cb(imgcopy_status status)
{
    static char trackmap[MAX_SECTORS+1];
    static int last_track;
    char *s;
    char *d;

    static const char bs2char[] =
    {
        ' ', '.', '-', '?', '*'
    };

    if(status.track == 0)
    {
        last_track = 0;
        return 0;
    }

    if(no_progress)
    {
        return 0;
    }

    if(last_track != status.track)
    {
        if(last_track)
        {
            printf("\r%2d: %-24s               \n", last_track, trackmap);
        }

        for(s = status.bam[status.track-1], d = trackmap; *s; s++, d++)
        {
            *d = bs2char[(int)*s];
        }
        *d = '\0';
        last_track = status.track;
    }

    trackmap[status.sector] = 
        bs2char[(status.read_result || 
                 status.write_result) ? bs_error : bs_copied];

    printf("\r%2d: %-24s%3d%%  %4d/%d", status.track, trackmap,
           100 * status.sectors_processed / status.total_sectors,
           status.sectors_processed, status.total_sectors);

    fflush(stdout);
    return 0;
}


//
// abort signal trap
//
static void ARCH_SIGNALDECL reset(int dummy)
{
    CBM_FILE fd_cbm_local;

    /*
     * remember fd_cbm, and make the global one invalid
     * so that no routine can call a cbm_...() routine
     * once we have cancelled another one
     */
    fd_cbm_local = fd_cbm;
    fd_cbm = CBM_FILE_INVALID;

    fprintf(stderr, "\nSIGINT caught X-(  Resetting IEC bus...\n");
#ifdef LIBIMGCOPY_DEBUG
    printDebugLibImgCounters(my_message_cb);
#endif
    imgcopy_cleanup();
    cbm_reset(fd_cbm_local);
    cbm_driver_close(fd_cbm_local);
    exit(1);
}

//
// main function 
//
int ARCH_MAINDECL main(int argc, char *argv[])
{
    imgcopy_settings *settings = imgcopy_get_default_settings();

    char *tm = NULL;
    char *src_arg;
    char *dst_arg;
    char *adapter = NULL;

    int  c;
    int  rv = 1;
    int  l;

    int src_is_cbm;
    int dst_is_cbm;

    struct option longopts[] =
    {
        { "help"       , no_argument      , NULL, 'h' },
        { "version"    , no_argument      , NULL, 'V' },
        { "adapter"    , required_argument, NULL, '@' },
        { "warp"       , no_argument      , NULL, 'w' },
        { "no-warp"    , no_argument      , &settings->warp, 0 },
        { "quiet"      , no_argument      , NULL, 'q' },
        { "verbose"    , no_argument      , NULL, 'v' },
        { "no-progress", no_argument      , NULL, 'n' },
        { "interleave" , required_argument, NULL, 'i' },
        { "start-track", required_argument, NULL, 's' },
        { "end-track"  , required_argument, NULL, 'e' },
        { "transfer"   , required_argument, NULL, 't' },
        { "bam-only"   , no_argument      , NULL, 'b' },
        { "bam-save"   , no_argument      , NULL, 'B' },
        { "drive-type" , required_argument, NULL, 'd' },
        { "retry-count", required_argument, NULL, 'r' },
        { "one-sided"  , no_argument      , NULL, '1' },
        { "two-sided"  , no_argument      , NULL, '2' },
        { "error-map"  , required_argument, NULL, 'E' },
        { NULL         , 0                , NULL, 0   }
    };

    const char shortopts[] ="hVwqbBt:i:s:e:d:r:2vnE:@:";

    while((c=getopt_long(argc, argv, shortopts, longopts, NULL)) != -1)
    {
        switch(c)
        {
            case 'h': help();
                      return 0;
            case 'V': printf("imgcopy %s\n", OPENCBM_VERSION);
                      return 0;
            case 'w': settings->warp = 1;
                      break;
            case 'q': if(verbosity > 0) verbosity--;
                      break;
            case 'v': verbosity++;
                      break;
            case 'n': no_progress = 1;
                      break;
            case 'i': settings->interleave = arch_atoc(optarg);
                      break;
            case 's': settings->start_track = atoi(optarg);
                      break;
            case 'e': settings->end_track = atoi(optarg);
                      break;
            case 't': tm = optarg;
                      break;
            case 'b': settings->bam_mode = bm_allocated;
                      break;
            case 'B': settings->bam_mode = bm_save;
                      break;
            case 'd': if(strcmp(optarg, "8050") == 0)
                      {
                          settings->drive_type = cbm_dt_cbm8050;
                      }
                      else if(strcmp(optarg, "8250") == 0)
                      {
                          settings->drive_type = cbm_dt_cbm8250;
                      }
                      else if(strcmp(optarg, "1001") == 0)
                      {
                          settings->drive_type = cbm_dt_sfd1001;
                      }
                      else if(strcmp(optarg, "1001") == 0)
                      {
                          settings->drive_type = cbm_dt_sfd1001;
                      }
                      else if(strcmp(optarg, "1541") == 0)
                      {
                          settings->drive_type = cbm_dt_cbm1541;
                      }
                      else if(strcmp(optarg, "1571") == 0)
                      {
                          settings->drive_type = cbm_dt_cbm1571;
                      }
                      else if(strcmp(optarg, "1581") == 0)
                      {
                          settings->drive_type = cbm_dt_cbm1581;
                      }
                      else if(strcmp(optarg, "2031") == 0)
                      {
                          settings->drive_type = cbm_dt_cbm2031;
                      }
                      else if(strcmp(optarg, "2040") == 0)
                      {
                          settings->drive_type = cbm_dt_cbm2040;
                      }
                      else if(strcmp(optarg, "3040") == 0)
                      {
                          settings->drive_type = cbm_dt_cbm3040;
                      }
                      else if(strcmp(optarg, "4031") == 0)
                      {
                          settings->drive_type = cbm_dt_cbm4031;
                      }
                      else if(strcmp(optarg, "4040") == 0)
                      {
                          settings->drive_type = cbm_dt_cbm4040;
                      }
                      else
                      {
                          my_message_cb(sev_fatal, "unknown drive type.");
                          hint(argv[0]);
                          exit(1);
                      }
                      break;
            case 'r': settings->retries = atoi(optarg);
                      break;
            case '1': settings->two_sided = 0;
                      break;
            case '2': settings->two_sided = 1;
                      break;
            case 'E': l = strlen(optarg);
                      if(strncmp(optarg, "always", l) == 0)
                      {
                          settings->error_mode = em_always;
                      }
                      else if(strncmp(optarg, "on_errors", l) == 0)
                      {
                          settings->error_mode = em_on_error;
                      }
                      else if(strncmp(optarg, "never", l) == 0)
                      {
                          settings->error_mode = em_never;
                      }
                      else
                      {
                          hint(argv[0]);
                          return 1;
                      }
                      break;
            case '@': if (adapter == NULL)
                          adapter = cbmlibmisc_strdup(optarg);
                      else
                      {
                          my_message_cb(sev_fatal, "--adapter/-@ given more than once.");
                          hint(argv[0]);
                          exit(1);
                      }
                      break;
            case 0:   break; // needed for --no-warp
            default : hint(argv[0]);
                      return 1;
        }
    }

	//printf("transfermode: %s\n", tm);
	settings->transfer_mode = imgcopy_get_transfer_mode_index(tm);
	if(settings->transfer_mode < 0)
	{
	    char *modes = imgcopy_get_transfer_modes();
	    char *m;

	    fprintf(stderr, "Unknown transfer mode: %s\nAvailable modes:\n", tm);

	    for(m = modes; *m; m+=(strlen(m)+1))
	    {
	        fprintf(stderr, "  %s\n", m);
	    }

	    free(modes);
	    return 1;
	}
	my_message_cb(3, "transfer mode is %d", settings->transfer_mode );

    if(optind + 2 != argc)
    {
        fprintf(stderr, "Usage: %s [OPTION]... [SOURCE] [TARGET]\n", argv[0]);
        hint(argv[0]);
        return 1;
    }

    src_arg = argv[optind];
    dst_arg = argv[optind+1];

    src_is_cbm = is_cbm(src_arg);
    dst_is_cbm = is_cbm(dst_arg);

    if(src_is_cbm == dst_is_cbm)
    {
        my_message_cb(0, "either source or target must be a CBM drive");
        return 1;
    }

    if(cbm_driver_open_ex(&fd_cbm, adapter) == 0)
    {
        /*
         * If the user specified auto transfer mode, find out
         * which transfer mode to use.
         */
        settings->transfer_mode = 
            imgcopy_check_auto_transfer_mode(fd_cbm,
                settings->transfer_mode,
                atoi(src_is_cbm ? src_arg : dst_arg));

        my_message_cb(3, "decided to use transfer mode %d", settings->transfer_mode );

        arch_set_ctrlbreak_handler(reset);

        if(src_is_cbm)
        {
            rv = imgcopy_read_image(fd_cbm, settings, atoi(src_arg), dst_arg,
                    my_message_cb, my_status_cb);
        }
        else
        {
            rv = imgcopy_write_image(fd_cbm, settings, src_arg, atoi(dst_arg),
                    my_message_cb, my_status_cb);
        }

        if(!no_progress && rv >= 0)
        {
            printf("\n%d blocks copied.\n", rv);
        }

        cbm_driver_close(fd_cbm);
        rv = 0;
    }
    else
    {
        arch_error(0, arch_get_errno(), "%s", cbm_get_driver_name_ex(adapter));
    }

    cbmlibmisc_strfree(adapter);
    free(settings);
    
    return rv;
}

