/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright (C) 2006 Wolfgang Moser (http://d81.de)
 *  Copyright (C) 1999 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 */

#include "cbmrpm41.h"
#include "libmisc.h"

#ifdef CBMRPM41_DEBUG
# define DEBUG_STATEDEBUG
#endif
#include "statedebug.h"

static unsigned char cbmrpm41[] = {
#include "cbmrpm41.inc"
};

#define _ASCII_PARAMETER_PASSING 1
#define _MINMAX_VALUES_PRINTOUT  0

static CBM_FILE fd;
static unsigned char drive;

const static unsigned short timerShotMain = sizeof(cbmDev_StartAddress) + offsetof(struct ExecBuffer_MemoryLayout, Timer24BitGroup);
const static unsigned short UcmdTblAddr   = sizeof(cbmDev_StartAddress) + offsetof(struct ExecBuffer_MemoryLayout, CommandVectorsTable_impl);

/*
 * The virtual 23.589 bit timer value is reconstructed from
 * a 16 bit timer and an 8 bit timer with a modulus of 187
 * with the help of the Chinese Remainder Theorem. All the
 * constants below are precalculated coefficients for a
 * modulus of 65536 for one timer and a modulus of 187 for
 * the other timer.
 */
#define Via1Timer2Max (256 * 256)           // 16 bit timer
#define Via2Timer2Max (185 + 2)             // == 187, with latch value 185

const static int Modulus  = Via1Timer2Max * Via2Timer2Max;
const static int V1T2rec1 =    27081;       // --16-Bit timer (inverse of a mod b)
const static int V1T2rec2 =      121;       // __/
const static int V2T2rec  =  8978432;       // 7.89-Bit timer (inverse of b mod a)

typedef struct
{
    unsigned int startValue, endValue, trueNumberOfIntervals;
} GroupOfMeasurements;


static void
help()
{
    printf
    (
        "Usage: cbmrpm41 [OPTION]... DRIVE\n"
        "High precision CBM-1541 rpm measurement\n"
        "\n"
        "  -h, --help                 display this help and exit\n"
        "  -V, --version              display version information and exit\n"
        "  -@, --adapter=plugin:bus   tell OpenCBM which backend plugin and bus to use\n"
        "\n"
        "  -j, --job=JOBID            measurement job to do:\n"
        "                                 1 - detailed RPM printout (default),\n"
        "                                 2 - track synchronization,\n"
        "                                 3 - RPM with linear regression and ANOVAR\n"
        "                                 4 - for RPM adjustment, with exponentially\n"
        "                                     moving average\n"
        "\n"
        "  -s, --status               display drive status after the measurements\n"
        "  -x, --extended             measure out a 40 track disk\n"
        "  -r, --retries=n            number of measurement retries for each track\n"
        "\n"
        "  -b, --begin-track=TRACK    set start track (1 <= start <= end)\n"
        "  -e, --end-track=TRACK      set end track  (start <= end <= 42)\n"
        "  -c, --sector=SECTOR        set trigger sector number (>=0, gets modulo\n"
        "                             limited by the max number of sectors for a track)\n"
        /*
        "\n"
        "  -q, --quiet                quiet output\n"
        "  -v, --verbose              control verbosity (repeatedly, up to 3 times)\n"
        "  -n, --no-progress          do not display progress information\n"
        */
        "\n"
    );
}

static void
hint(char *s)
{
    fprintf(stderr, "Try `%s' -h for more information.\n", s);
}

static void ARCH_SIGNALDECL
handle_CTRL_C(int dummy)
{
    CBM_FILE fd_cbm_local;

    /*
     * remember fd_cbm, and make the global one invalid
     * so that no routine can call a cbm_...() routine
     * once we have cancelled another one
     */
    fd_cbm_local = fd;
    fd = CBM_FILE_INVALID;

    fprintf(stderr, "\nSIGINT caught, resetting IEC bus...\n");

    DEBUG_PRINTDEBUGCOUNTERS();

    arch_sleep(1);
    cbm_reset(fd_cbm_local);

#if 0   // reset automatically restores the VIA shift register
    arch_usleep(100000);
    fprintf(stderr, "Emergency resetting VIA2 shift register to default.\n");
    cbm_exec_command(fd, drive, CmdBuffer, sizeof(CmdBuffer));
#endif

    cbm_driver_close(fd);
    exit(1);
}

static int
cbm_sendUxCommand(CBM_FILE HandleDevice, unsigned char DeviceAddress, enum UcmdVectorNames UxCommand)
{
    char UxCmdBuffer[3]="U_";

    UxCmdBuffer[1]=UxCommand;
    return cbm_exec_command(HandleDevice, DeviceAddress, UxCmdBuffer, 2);
}

static unsigned int
reconstruct_v32bitInc(struct Timer24bitValues TimerRegisters)
{
    /*
     * The virtual 23.589 bit timer can further be extended to
     * 32 bits with a software method. But this works only
     * correctly, as long as two consecutive time measurements
     * (from the very same timing source of course) always
     * differ in less than a complete wrap around of the
     * virtual timer. On a base clock of 1Mhz and the Modulus
     * of 256*256*187 this is a time window of somewhat around
     * 12s (Modulus / 1MHz).
     */
    static int lastVTimer = 0;
    static unsigned int ModulusDecrementor = 0;

    register int vTimer;

// #define Timer23Debug 1
#if Timer23Debug >= 1
    printf("Plain 23.589 bit timer values read: 0x%02x 0x%02x 0x%02x\n",
           TimerRegisters.V2T2__LOW , TimerRegisters.V1T2__LOW, TimerRegisters.V1T2_HIGH);
#endif

    vTimer   = TimerRegisters.V1T2_HIGH;
    vTimer <<= 8;
    vTimer  |= TimerRegisters.V1T2__LOW;
    vTimer  *= V1T2rec1;
    vTimer  %= Modulus;
    vTimer  *= V1T2rec2;
    vTimer  %= Modulus;

    vTimer  += V2T2rec * TimerRegisters.V2T2__LOW;
    vTimer  %= Modulus;

#if Timer23Debug >= 1
    printf("Reconstructed 23.589 bit timer value: 0x%06x / %8d\n", vTimer, vTimer);
#endif

    // obey that the timer decrements on each tick
    if ( vTimer > lastVTimer )
    {
        // the timer increased, thus we have got a wrap
        // around ==> decrement the 32 bits software
        // timer by the modulus of the virtual timer
        ModulusDecrementor -= Modulus;
    }

    lastVTimer = vTimer;

    // by taking the (( 2^n ) - 1 )-Inverse, we "convert"
    // the timer an increasing one instead of decreasing
    // with each tick
    return ~(ModulusDecrementor + vTimer);
}

static int
measure_2cyleJitter(CBM_FILE HandleDevice, unsigned char DeviceAddress,
                    unsigned char diskTrack, unsigned char sector, unsigned char count,
                    GroupOfMeasurements *pDeltaGroup,
                    int printDeltas)
{
    char cmd[10];
    unsigned char insts[40];
    unsigned int mNo, timerValue, lastTvalue;
#if _MINMAX_VALUES_PRINTOUT
    unsigned int dMin=~0, dMax=0;
#endif
    struct Timer24bitValues T24Sample;

    SETSTATEDEBUG((void)0);

#if _ASCII_PARAMETER_PASSING
    // must be: "Ux <track> <sector>"
    // sprintf(cmd, "U%c %d %d", ExecuteJobInBuffer, i, i & 0x0f);
    sprintf(cmd, "U%c %d %d", ExecuteJobInBuffer, diskTrack, sector);
#else
    // must be: "Ux<track><sector>" with directly encoded bytes
    sprintf(cmd, "U%c%c%c", ExecuteJobInBuffer, diskTrack, sector);
#endif

    pDeltaGroup->trueNumberOfIntervals = 0;

    // for each track do 1 initialisation and then
    // several measurements
    timerValue = 0;
    for(mNo = 0; mNo <= count; mNo++)
    {
        lastTvalue = timerValue;

#if _ASCII_PARAMETER_PASSING
        if( cbm_exec_command(HandleDevice, DeviceAddress, cmd, strlen(cmd))
            != 0) return 1;
#else
        if( cbm_exec_command(HandleDevice, DeviceAddress, cmd, 4)
            != 0) return 1;
#endif
        SETSTATEDEBUG((void)0);

        // wait for job to finish
        if( cbm_device_status(HandleDevice, DeviceAddress, insts, sizeof(insts)) )
        {
            printf("%s\n", insts);
        }

        if( cbm_download(HandleDevice, DeviceAddress,
                     timerShotMain, (unsigned char *) & T24Sample,
                     sizeof(T24Sample))
             != sizeof(T24Sample)) return 1;

        // read out sample that was shot by the jobcode
        timerValue = reconstruct_v32bitInc(T24Sample);

        if(mNo > 0){
            lastTvalue = timerValue - lastTvalue;

            // increase by the number of overflows
            pDeltaGroup->trueNumberOfIntervals +=
                (lastTvalue + 100000) / 200000;

            if( printDeltas ) printf("%6u ", lastTvalue);
#if _MINMAX_VALUES_PRINTOUT
            if(lastTvalue > dMax) dMax = lastTvalue;
            if(lastTvalue < dMin) dMin = lastTvalue;
#endif
        }
        else
        {
            pDeltaGroup->startValue = timerValue;
            if( printDeltas ) printf(" %10u ||", timerValue);
        }
    }
#if _MINMAX_VALUES_PRINTOUT
    if( printDeltas ) printf(" %6u..%6u=%2u", dMin, dMax, dMax - dMin);
#endif

    pDeltaGroup->endValue = timerValue;
    return 0;
}

static unsigned char
limitSectorNo41(register unsigned char track, int secno)
{
    // 17/18, 24/25, 30/31

    if(track > 24)      // unrolled bipartition algorithm
    {                   // could be done with a shifting bitmask also
        if(track > 30)  // or maybe by some sort of non-linear
        {               // hashing and a switch-case
            return secno % 17;
        }
        else
        {
            return secno % 18;
        }
    }
    else    // track is:  <= 24
    {
        if(track > 17)
        {
            return secno % 19;
        }
        else    // track is:  <= 17
        {
            return secno % 21;
        }
    }
}

static int
do_RPMmeasurment(unsigned char start, unsigned char end, int sec, unsigned char retries)
{
    unsigned char track;
    GroupOfMeasurements measureGroup;
    float meanTime;

    printf(" TR | timer abs. ||delta1,delta2,...                 |mean delta|mean rpm\n"
           "  # |      (~us) || (~us), (~us),...                 |     (~us)| (1/min)\n"
           "----+------------++------+---------------------------+----------+---------\n");
    for(track = start; track <= end; track++)
    {
        printf(" %2d |", track);
        if( measure_2cyleJitter(fd, drive, track, limitSectorNo41(track, sec), retries,
            &measureGroup, 1
            ) != 0) return 1;

        meanTime = (float)(measureGroup.endValue - measureGroup.startValue) / measureGroup.trueNumberOfIntervals;
        printf(" %8.1f%c| %7.3f\n", meanTime, (retries == measureGroup.trueNumberOfIntervals) ? '\'' : 'c', 60000000.0 / meanTime);
    }

    return 0;
}

static int
do_SKEWmeasurment(unsigned char start, unsigned char end, int sec, unsigned char retries)
{
    unsigned char track;
    GroupOfMeasurements measureGroup, prevMeasureGroup;
    float meanDelta, skewDelta;

    printf(" Tracks | Sc |mean delta|meanrpm||  skew| skew mod|  degree|  radians\n"
           "    (#) |    |     (~us)|(1/min)|| (~us)|    (~us)|     (o)|    (rad)\n"
           "--------+----+----------+-------++------+---------+--------+----------\n");


    track = start;

    if(track <= end)
    {
        if( measure_2cyleJitter(fd, drive, track, limitSectorNo41(track, sec), retries,
            &measureGroup, 0
            ) != 0) return 1;

        prevMeasureGroup.endValue   = measureGroup.startValue;
        prevMeasureGroup.startValue = ((measureGroup.startValue) << 1) -  measureGroup.endValue;
        prevMeasureGroup.trueNumberOfIntervals = measureGroup.trueNumberOfIntervals;

        switch(1)
        {
            do {
                prevMeasureGroup = measureGroup;

                if( measure_2cyleJitter(fd, drive, track, limitSectorNo41(track, sec), retries,
                    &measureGroup, 0
                    ) != 0) return 1;
            case 1:

                // meanDelta = (float)(lastT - firstT + prevLastT - prevFirstT) / (retries << 1);

                meanDelta = (float)(measureGroup.endValue - measureGroup.startValue +
                                    prevMeasureGroup.endValue - prevMeasureGroup.startValue) /
                            (measureGroup.trueNumberOfIntervals + prevMeasureGroup.trueNumberOfIntervals);
                printf(" %2d..%2d | %2d |%10.3f|%7.3f||", track - 1, track, limitSectorNo41(track, sec), meanDelta,
                       60000000.0f * measureGroup.trueNumberOfIntervals /
                       (measureGroup.endValue - measureGroup.startValue));

                skewDelta = (float)fmod(measureGroup.startValue - prevMeasureGroup.endValue, meanDelta);
                // move from range 0...1 into range -0.5...0.5
                if( (2 * skewDelta) > meanDelta ) skewDelta -= meanDelta;
                printf("%6u|%9.1f|", measureGroup.startValue - prevMeasureGroup.endValue, skewDelta);

                skewDelta /= meanDelta;     // relative fractional value 0...1.0
                printf("%8.3f|%9.6f\n", skewDelta * 360, skewDelta * 2 * 3.14159265358979323846);

                track++;
            } while(track <= end);
        }
    }

    return 0;
}

static int
do_RPMregression(unsigned char start, unsigned char end, int sec, unsigned char retries)
{
    int i, x;
    unsigned char track;
    GroupOfMeasurements measureGroup;
    unsigned int lastTValue;

    long long int Sy, Syy, Sxy;
    unsigned int  Sx, Sxx;
    double meanTime, variance;

    printf(" TR | measurment points |   rotation | variance | meanrpm | variance\n"
           "  # |      abscissa (x) | time (~us) |   (us^2) | (1/min) |(1/min^2)\n"
           "----+-------------------|------------+----------|---------+----------\n");
    for(track = start; track <= end; track++)
    {

        x = 0;
        if( measure_2cyleJitter(fd, drive, track, limitSectorNo41(track, sec), 0,
            &measureGroup, 0
            ) != 0) return 1;
        printf(" %2d | %2d", track, x);

        Sx  = Sxx = 0;      // initialise least squares summarization terms
        Sxy = 0ll;
        Sy  = measureGroup.startValue;
        Syy = (long long int)measureGroup.startValue * measureGroup.startValue;

        for( i = 1; i <= retries; i++)
        {
            lastTValue = measureGroup.startValue;

            if( measure_2cyleJitter(fd, drive, track, limitSectorNo41(track, sec), 0,
                &measureGroup, 0
                ) != 0) return 1;

            x += (measureGroup.startValue - lastTValue + 100000) / 200000;

            // printf("%9u/%u|", measureGroup.startValue, x);
            printf(" %2d", x);

            Sx  += x;                           // can be calculated easily
            Sxx += x * x;    // maybe also calculatable (?)
            Sy  += measureGroup.startValue;
            Syy += (long long int)measureGroup.startValue * measureGroup.startValue;
            Sxy += (long long int)measureGroup.startValue * x;

            // printf("\n\tValue: %10d\tSums: %d, %d, %I64d, %I64d, %I64d\n",measureGroup.startValue , Sx, Sxx, Sy, Syy, Sxy);
        }

        // http://en.wikipedia.org/wiki/Linear_regression
        meanTime  = (double)( i * Sxy - Sx * Sy ) / ( i * Sxx - Sx * Sx );    // b
        // a = (Sy - meanTime * Sx) / i;

        // http://www.forst.tu-dresden.de/Biometrie/formeln/form10.html
        // the following formulae were derived from the page above, where
        // all the Q...-terms were multiplicated by n to eliminate as
        // much divisions as possible
        variance  = (double)(i * Sxy - Sx * Sy);
        variance *= -variance;                      // make the term negative
        variance /= (double)(i * Sxx - Sx * Sx);
        variance += (double)(i * Syy - Sy * Sy);
        variance /= (double)(i * (i - 2));
        // useful also for the (division reduced) variance formulae:
        // http://fresh.t-systems-sfr.com/linux/src/xmstat-2.2.tar.gz:t/xmstat-2.2/src/s_cb.cc

        printf(" | %10.3f | %8.3f", meanTime, variance);

        
        variance /= meanTime;   // relativate variance
        meanTime  = 60e6 / meanTime;
        variance *= meanTime;  // and accomodate it to the new meanTime again

        printf(" | %7.3f | %8.6f\n", meanTime, variance);
    }

    return 0;
}

static int
do_RPMadjustment(unsigned char track, unsigned char dummy, int sec, unsigned char retries)   // end track not needed
{
    GroupOfMeasurements measureGroup;
    const float alpha[] = { 0.22f, 0.10f, 0.047f, 0.022f };

    // this is a trick for defining some intermediate sort
    // of constants between lexicalic ones (#define) and
    // typed ones (const). The latter cannot be used for
    // array size declarations, but:  sizeof(aSize)  can.
    typedef char aSize[sizeof(alpha) / sizeof(float)];

    float RPM, expMovAv[sizeof(aSize)];
    unsigned int delta;
    int i, j, alph;

    printf("  run |  delta |     RPM | exponentional moving averages\n"
           "  No. |  (~us) | (1/min) |");
    for(alph = 0; alph < sizeof(aSize); ++alph) printf(" a=%5.3f", alpha[alph]);
    printf("\n------+--------+---------+---------------------------------\n");

    if( measure_2cyleJitter(fd, drive, track, limitSectorNo41(track, sec), 0,
        &measureGroup, 0
        ) != 0) return 1;
    delta = measureGroup.startValue;
    if( measure_2cyleJitter(fd, drive, track, limitSectorNo41(track, sec), 0,
        &measureGroup, 0
        ) != 0) return 1;
    delta = measureGroup.startValue - delta;
    // overflow correction, needs integer division
    delta /= (delta + 100000) / 200000;
    RPM = 60000000.0f / delta;

    for(alph = 0; alph < sizeof(aSize); ++alph) expMovAv[alph] = RPM;

    for(i = 0; i < retries; ++i)
    {
        for(j = 1; j <= 100; ++j)
        {
            delta = measureGroup.startValue;
            if( measure_2cyleJitter(fd, drive, track, limitSectorNo41(track, sec), 0,
                &measureGroup, 0
                ) != 0) return 1;
    
            delta = measureGroup.startValue - delta;
    
            // overflow correction, needs integer division
            delta /= (delta + 100000) / 200000;
    
            RPM = 60000000.0f / delta;
            printf("\r%5d | %6d | %7.3f |", i*100+j, delta, RPM);
    
            // Exponential moving average:
            // see: http://en.wikipedia.org/wiki/Weighted_moving_average
            // and: http://www.itl.nist.gov/div898/handbook/pmc/section4/pmc431.htm
            for(alph = 0; alph < sizeof(aSize); ++alph)
            {
                expMovAv[alph] = expMovAv[alph] + alpha[alph] * ( RPM - expMovAv[alph] );
                printf(" %7.3f", expMovAv[alph]);
            }
        }
        printf("\n");
    }
    // printf("\n");

    return 0;
}


int ARCH_MAINDECL
main(int argc, char *argv[])
{
    int status = 0;
    char cmd[40];
    unsigned char job = 1, begintrack = 1, endtrack = 35, retries = 5;
    char *arg;
    char *adapter = NULL;
    int sector = 0, berror = 0;
    int option;

    struct option longopts[] =
    {
        { "help"       , no_argument      , NULL, 'h' },
        { "version"    , no_argument      , NULL, 'V' },
        { "adapter"    , required_argument, NULL, '@' },
        { "job"        , no_argument      , NULL, 'j' },
        { "retries"    , required_argument, NULL, 'r' },
        { "extended"   , no_argument      , NULL, 'x' },
        { "retries"    , required_argument, NULL, 'r' },
        { "begin-track", required_argument, NULL, 'b' },
        { "end-track"  , required_argument, NULL, 'e' },
        { "sector"     , required_argument, NULL, 'c' },
/*
        { "quiet"      , no_argument      , NULL, 'q' },
        { "verbose"    , no_argument      , NULL, 'v' },
        { "no-progress", no_argument      , NULL, 'n' },
*/
        { NULL         , 0                , NULL, 0   }
    };

    // const char shortopts[] ="hVj:sr:xb:e:c:qvn";
    const char shortopts[] ="hVj:sxr:b:e:c:@:";


    while((option = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1)
    {
        switch(option)
        {
            case 'h': help();
                      return 0;
            case 'V': printf("cbmrpm41 Version %s\n", OPENCBM_VERSION ", built on " __DATE__ " at " __TIME__ "\n");
                      return 0;
            case 'j': job = arch_atoc(optarg);
                      break;
            case 's': status = 1;
                      break;
            case 'x': begintrack = 1;
                      endtrack = 40;
                      break;

            case 'r': retries = arch_atoc(optarg);
                      if(retries<1)       retries =  1;
                      else if(retries>63) retries = 63;
                      break;
            case 'b': begintrack = arch_atoc(optarg);
                      break;
            case 'e': endtrack = arch_atoc(optarg);
                      break;
            case 'c': sector = atoi(optarg);
                      break;
            case '@': if (adapter == NULL)
                          adapter = cbmlibmisc_strdup(optarg);
                      else
                      {
                          fprintf(stderr, "--adapter/-@ given more than once.");
                          help();
                          return 0;
                      }
                      break;
            default : hint(argv[0]);
                      return 1;
        }
    }

    if(optind + 1 != argc)
    {
        fprintf(stderr, "Usage: %s [OPTION]... DRIVE\n", argv[0]);
        hint(argv[0]);
        return 1;
    }

    arg = argv[optind++];
    drive = arch_atoc(arg);
    if(drive < 8 || drive > 11)
    {
        fprintf(stderr, "Invalid drive number (%s)\n", arg);
        return 1;
    }
    if(begintrack < 1)
    {
        fprintf(stderr, "Beginning track is less than 1, it should be 1 or greater.\n");
        return 1;
    }
    if(endtrack > 42)
    {
        fprintf(stderr, "Ending track is greater than 42, it should be 42 or less.\n");
        return 1;
    }
    if(begintrack > endtrack)
    {
        fprintf(stderr, "Beginning track is greater than ending track, it should be less or equal.");
        return 1;
    }
    if(sector < 0)
    {
        fprintf(stderr, "Sector numbers less than zero are not allowed.");
        return 1;
    }


    SETSTATEDEBUG((void)0);
    printf("Please remove any diskettes used with production data on it. Insert a freshly\n"
           "formatted disk into drive %d; you can format a disk with e.g. the command:\n\n"
           "        cbmforng -o -v %d freshdisk,fd\n\n"
           "If you desperately need to examine a production disk or even an original\n"
           "diskette, then please protect the disk with a write protect adhesive label.\n\n"
           "Press <Enter>, when ready or press <CTRL>-C to abort.\r", drive, drive);
    getchar();

    if(cbm_driver_open_ex(&fd, adapter) == 0) do
    {
        arch_set_ctrlbreak_handler(handle_CTRL_C);

        SETSTATEDEBUG((void)0);
        if( cbm_upload(fd, drive, sizeof(cbmDev_StartAddress), cbmrpm41, sizeof(cbmrpm41))
            != sizeof(cbmrpm41)) break;

        // location of the new U vector user commands table
        sprintf(cmd, "%c%c", UcmdTblAddr & 0xFF, UcmdTblAddr >> 8);
        // install the new U vector table
        SETSTATEDEBUG((void)0);
        if( cbm_upload(fd, drive, sizeof(cbmDev_UxCMDtVector), cmd, 2)
            != 2) break;

        // execute Ux command behind the symbolic name Init23_BitTimersStd
        SETSTATEDEBUG((void)0);
        if( cbm_sendUxCommand(fd, drive, Init23_BitTimersStd)
            != 0) break;

        // read disk ID and initialise other parameters
        // from the currently inserted disk into the
        // drive's RAM locations
        SETSTATEDEBUG((void)0);
        if( cbm_exec_command(fd, drive, "I0", 2)
            != 0) break;

        SETSTATEDEBUG((void)0);
        berror = cbm_device_status(fd, drive, cmd, sizeof(cmd));
        if(berror && status)
        {
            printf("%s\n", cmd);
        }

        switch(job)
        {
        case 4:
            if( do_RPMadjustment (begintrack, endtrack, sector, retries)
                != 0 ) continue;    // jump to begin of do{}while(0);
            break;
        case 3:
            if( do_RPMregression (begintrack, endtrack, sector, retries)
                != 0 ) continue;    // jump to begin of do{}while(0);
            break;
        case 2:
            if( do_SKEWmeasurment(begintrack, endtrack, sector, retries)
                != 0 ) continue;    // jump to begin of do{}while(0);
            break;
        default:
            if( do_RPMmeasurment (begintrack, endtrack, sector, retries)
                != 0 ) continue;    // jump to begin of do{}while(0);
        }

        if( cbm_sendUxCommand(fd, drive, ResetVIA2ShiftRegConfig)
            != 0 ) break;
        if( cbm_sendUxCommand(fd, drive,      ResetUxVectorTable)
            != 0 ) break;

        if( cbm_exec_command(fd, drive, "I", 2)
            != 0 ) break;

        if(!berror && status)
        {
            cbm_device_status(fd, drive, cmd, sizeof(cmd));
            printf("%s\n", cmd);
        }
        cbm_driver_close(fd);
        cbmlibmisc_strfree(adapter);
        return 0;
    } while(0);
    else
    {
        arch_error(0, arch_get_errno(), "%s", cbm_get_driver_name_ex(adapter));
        cbmlibmisc_strfree(adapter);
        return 1;
    }
    // if the do{}while(0) loop is exited with a break, we get here
    arch_error(0, arch_get_errno(), "%s", cbm_get_driver_name_ex(adapter));
    cbm_reset(fd);
    cbmlibmisc_strfree(adapter);
    cbm_driver_close(fd);
    return 1;
}
