/*
 * xum1541 IEE-488 routines
 * Copyright (c) 2011          diddl   <diddl@t-winkler.net>
 *
 * Revision 0.91 2011/04/16    diddl
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#include "xum1541.h"
#include "ieee.h"

//#define DEBUG_LED

// XXX These port lines are hard-coded, which means it will only work with
// the ZoomFloppy design. This should be fixed to use the board-*.[ch] files
// like for other protocols.
#define DDRdef      DDRC
#define PORTdef     PORTC
#define PINdef      PINC

#ifndef DDRA
 #define DDRA       DDRdef
 #define PORTA      PORTdef
 #define PINA       PINdef
#endif

#ifndef DDRD
 #define DDRD       DDRdef
 #define PORTD      PORTdef
 #define PIND       PINdef
#endif

#define IeeeGetPort(a)  (a & 0xf0) 
#define IeeeGetBit(a)   (a & 0xf) 
#define IeeeIsPort(a,b) ((a & 0xf0) == b) 

#define IeeeDdr(a)      (IeeeIsPort(a,0xA0) ? &DDRA  : \
    IeeeIsPort(a,0xC0) ? &DDRC  : IeeeIsPort(a,0xD0) ? &DDRD  : &DDRB)
#define IeeePort(a)     (IeeeIsPort(a,0xA0) ? &PORTA : \
    IeeeIsPort(a,0xC0) ? &PORTC : IeeeIsPort(a,0xD0) ? &PORTD : &PORTB)
#define IeeePin(a)      (IeeeIsPort(a,0xA0) ? PINA   : \
    IeeeIsPort(a,0xC0) ? PINC   : IeeeIsPort(a,0xD0) ? PIND     : PINB)

#define IeeeInp(a,b)    a
#define IeeeOut(a,b)    b
#define IeeeIsSame(a,b) (a == b)

#define IeeeSetReg(state,port,bit) {if (state) *(port) |= (1<<bit);    else *(port) &= ~(1<<bit); }


// set ddr
#define IeeeSetDdr(state,port)     {IeeeSetReg(state,IeeeDdr(port),IeeeGetBit(port))}

// set output pin if separate IO
#define IeeeSetPort(state,port)    {IeeeSetReg(state,IeeePort(port),IeeeGetBit(port))}

// set output pin if single IO
#define IeeeSetSame(state,port)    {if(state){IeeeSetDdr(0,port);IeeeSetPort(1,port);}else{IeeeSetPort(0,port);IeeeSetDdr(1,port);}}

// set/reset pullup
#define IeeeSetPullupSame(state,port) {IeeeSetDdr(0,port);IeeeSetPort(state,port);}
#define IeeeSetPullup(state,in,out)   {if(IeeeIsSame(in,out)) {IeeeSetPullupSame(state,in);} else {IeeeSetPort(state,in);}}

// set IEEE pin to input
#define IeeeSetInp(in,out)            {IeeeSet(1,in,out);} 

// set direction register and pullup
#define IeeeInitIO(in,out)            {if(IeeeIsSame(in,out)) {IeeeSetSame(1,in);} else {IeeeSetSame(0,out);IeeeSetSame(1,in);}}

// set IEEE output line
#define IeeeSet(state,in,out)         {if(IeeeIsSame(in,out)) {IeeeSetSame(state,in);} else {IeeeSetPort(!state,out);} }

// get IEEE input line
#define IeeeGet(port)                 (IeeePin(port) & (1<<IeeeGetBit(port)))

// IEEE data lines
#define IeeeDataDdr(port, ddr)  *(IeeeDdr(port)) = ddr 
#define IeeeDataPort(port, by)  *(IeeePort(port)) = by
#define IeeeDataOut(by)         IeeeDataDdr(IEEE_DATA_IO, 0x00); \
    IeeeDataPort(IEEE_DATA_IO, by);IeeeDataDdr(IEEE_DATA_IO, ~by);
#define IeeeDataIn()            IeeeDataDdr(IEEE_DATA_IO, 0x00); \
    IeeeDataPort(IEEE_DATA_IO, 0xff)

//
// set output line
//
#define IeeeAtn(state)  IeeeSet(state, IEEE_ATN_I,  IEEE_ATN_O)
#define IeeeDav(state)  IeeeSet(state, IEEE_DAV_I,  IEEE_DAV_O)
#define IeeeEoi(state)  IeeeSet(state, IEEE_EOI_I,  IEEE_EOI_O)
#define IeeeIfc(state)  IeeeSet(state, IEEE_IFC_I,  IEEE_IFC_O)
#define IeeeSrq(state)  IeeeSet(state, IEEE_SRQ_I,  IEEE_SRQ_O)
#define IeeeRen(state)  IeeeSet(state, IEEE_REN_I,  IEEE_REN_O)
#define IeeeNdac(state) IeeeSet(state, IEEE_NDAC_I, IEEE_NDAC_O)
#define IeeeNrfd(state) IeeeSet(state, IEEE_NRFD_I, IEEE_NRFD_O)

#define IeeeLED(state)  IeeeRen(state)

//
// read input line
//
#define IEEE_ATN    (IeeeGet(IEEE_ATN_I))
#define IEEE_IFC    (IeeeGet(IEEE_IFC_I))
#define IEEE_EOI    (IeeeGet(IEEE_EOI_I))
#define IEEE_DAV    (IeeeGet(IEEE_DAV_I))
#define IEEE_NRFD   (IeeeGet(IEEE_NRFD_I))
#define IEEE_NDAC   (IeeeGet(IEEE_NDAC_I))
#define IEEE_REN    (IeeeGet(IEEE_REN_I))
#define IEEE_SRQ    (IeeeGet(IEEE_SRQ_I))
#define IEEE_DATA   (IeeePin(IEEE_DATA_IO))

#define    ATN_DELAY    90 // Bus delay after ATN in us

// STATICS
//static uint8_t ieee_device;             // current device#
//static uint8_t ieee_devtyp;             // DT_2030,DT_4040,DT_8050,DT_8250
static uint8_t ieee_status;             // bit0=timeout out; bit1=timeout in; 
                                        // bit6=EOI, bit7=device not present
static volatile uint16_t ieee_timer;
static int16_t last_byte;               // -1=kein byte
static volatile bool ieee_listen;
static volatile bool ieee_talk;

// Internal function definitions
static void IeeeInitLines(void);
static bool IeeeDetect(void);
static int8_t IeeeListen(uint8_t dev, uint8_t sa);
static int8_t IeeeTalk(uint8_t dev, uint8_t sa);
static int8_t IeeeUntalk(void);
static int8_t IeeeUnlisten(void);
static int8_t IeeeOpen(uint8_t dev, uint8_t sa, char *);
static int8_t IeeeClose(uint8_t dev, uint8_t sa);
static int8_t IeeeBsout(uint8_t by);
static uint8_t IeeeBasin(void);

static void ieee_reset(bool forever);
static uint16_t ieee_raw_write(uint16_t len, uint8_t flags);
static uint16_t ieee_raw_read(uint16_t len);
static bool ieee_wait(uint8_t line, uint8_t state);
static uint8_t ieee_poll(void);
static void ieee_setrelease(uint8_t set, uint8_t release);

static struct ProtocolFunctions ieeeFunctions = {
    .cbm_reset = ieee_reset,
    .cbm_raw_write = ieee_raw_write,
    .cbm_raw_read = ieee_raw_read,
    .cbm_wait = ieee_wait,
    .cbm_poll = ieee_poll,
    .cbm_setrelease = ieee_setrelease,
};

//
// IEEE timer solution without interrupt 
//
static bool IsTimeout(void)
{
    static uint8_t cnt = 0;

    _delay_us(10);
    if(++cnt > 99)
    {
        // ~ every ms
        cnt = 0;
        if(ieee_timer) --ieee_timer;

    }
    wdt_reset();

    return (ieee_timer == 0);
}

static void DelayMs(uint16_t ms)
{
    ieee_timer = ms;
    while(!IsTimeout());
}

//
// LED blinker for debugging
//
#ifdef DEBUG_LED
static void debug_LED_blink(uint8_t cnt)
{
    while(1)
    {
        if(cnt == 0) break;

        IeeeLED(0);
        DelayMs(330);
        IeeeLED(1);
        DelayMs(165);

        cnt--;
    }
    DelayMs(500);
}
#endif

//
// initialize all IO lines (set DDR and pullup)
//
static void IeeeInitLines(void)
{
    IeeeDataIn();                                    // DATAPORT INPUT!
    IeeeInitIO(IEEE_ATN_I, IEEE_ATN_O);
    IeeeInitIO(IEEE_DAV_I, IEEE_DAV_O);
    IeeeInitIO(IEEE_EOI_I, IEEE_EOI_O);
    IeeeInitIO(IEEE_IFC_I, IEEE_IFC_O);
    IeeeInitIO(IEEE_SRQ_I, IEEE_SRQ_O);
    IeeeInitIO(IEEE_REN_I, IEEE_REN_O);
    IeeeInitIO(IEEE_NDAC_I, IEEE_NDAC_O); 
    IeeeInitIO(IEEE_NRFD_I, IEEE_NRFD_O);

    ieee_status = 0;
    ieee_listen = false;
    ieee_talk   = false;
    last_byte   = -1;                // -1=kein byte
}

//
// Check if device is connected to IEEE-488 port (device pullups)
// This function needs to run quickly so it won't delay IEC initialization
// if no IEEE device is attached and powered on.
//
static bool IeeeDetect(void)
{
    bool flg;

#ifdef DEBUG_LED
    debug_LED_blink(10);
#endif

    // reset pullups and test IO
    IeeeDav(0);
    IeeeEoi(0);
    //IeeeRen(0);
    IeeeSetPullup(0, IEEE_DAV_I, IEEE_DAV_O);
    IeeeSetPullup(0, IEEE_EOI_I, IEEE_EOI_O);
    //IeeeSetPullup(0, IEEE_REN_I, IEEE_REN_O);
    DELAY_US(100);
    //flg = (IEEE_REN && IEEE_DAV && IEEE_EOI);
    flg = (IEEE_DAV && IEEE_EOI);

    if(flg)
    {
#ifdef DEBUG_LED
        debug_LED_blink(5);
#endif
    }
    else
    {
#ifdef DEBUG_LED
        debug_LED_blink(1);
#endif
    }

    return (flg);
}

// Initialize and claim the bus if an IEEE device is powered on.
struct ProtocolFunctions *
ieee_init(void)
{
    bool devicePresent;

    IeeeInitLines();
    DELAY_US(100);

    // Only claim the bus if we detect a device present
    devicePresent = IeeeDetect();
    if (devicePresent) {
        IeeeInitLines();
        return &ieeeFunctions;
    } else
        return NULL;
}

/*
 * All exit paths have to take ~200 us total for the timing in
 * wait_for_free_bus() to be correct. Once we're past the point of finding
 * an active drive (DATA set after ATN is set), we have hit 200 us.
 */
static uint8_t check_if_bus_free(void)
{
    DELAY_US(10);

    if(!IEEE_NRFD || !IEEE_NDAC)
    {
        DELAY_US(190);
        //debug_LED_blink(10);
        return 0;
    }

    //debug_LED_blink(2);

    IeeeAtn(0);                                    // ATN!
    DELAY_US(10);

    if(IEEE_NRFD && IEEE_NDAC)
    {
        // device not present!
        IeeeAtn(1);                                // release ATN
        DELAY_US(180);
        //debug_LED_blink(8);
        return 0;
    }

    IeeeAtn(1);                                    // release ATN
    DELAY_US(50);
    if(!IEEE_NRFD || !IEEE_NDAC)
    {
        DELAY_US(100);
        if(!IEEE_NRFD || !IEEE_NDAC)
        {
            DELAY_US(30);
            return 0;
        }
    }

    DELAY_US(130);
    //debug_LED_blink(8);
    return (IEEE_NRFD && IEEE_NDAC);
}

/*
 * Wait up to 1.5 secs to see if any drive answers ATN toggle.
 * Technically, we only have to wait up to IEC_T_AT (1 ms) but we're
 * being generous here.
 */
static void
wait_for_free_bus(bool forever)
{
    uint16_t i;

    for (i = (uint16_t)(XUM1541_TIMEOUT * 5000); i != 0 || forever; i--) {
        /*
         * We depend on the internal delays within this function to be sure
         * the whole loop takes long enough. If there is no device, this
         * takes 200 us per try (1.5 sec total timeout).
         *
         * In the minimal case (DATA held the whole time), it only delays
         * for 50 us per try (375 ms total timeout). This is still much
         * more than IEC_T_AT.
         */
        if (check_if_bus_free())
            return;

        // Bail out early if host signalled an abort.
        if (!TimerWorker())
            return;
    }
    DEBUGF(DBG_ERROR, "wait4free bus to\n");
}

static void ieee_reset(bool forever)
{
    // Pull IFC active for 50 ms, then look for a device responding.
    IeeeInitLines();
    IeeeIfc(0);
    DelayMs(50);
    IeeeIfc(1);

    wait_for_free_bus(forever);
}

static uint8_t ieee_poll(void)
{
    uint8_t rv = 0;

    if (!IEEE_NDAC)
        rv |= IEE_NDAC;
    if (!IEEE_NRFD)
        rv |= IEE_NRFD;
    if (!IEEE_ATN)
        rv |= IEE_ATN;
    if (!IEEE_IFC)
        rv |= IEE_IFC;
    if (!IEEE_DAV)
        rv |= IEE_DAV;
    if (!IEEE_EOI)
        rv |= IEE_EOI;
/*
    if (!IEEE_REN)
        rv |= IEE_REN;
    if (!IEEE_SRQ)
        rv |= IEE_SRQ;
*/
    return rv;
}

static bool ieee_wait(uint8_t line, uint8_t state)
{
    return true;
}

static void ieee_setrelease(uint8_t set, uint8_t release)
{
    // not supported yet
}

/* 
 * Write bytes to the drive via the CBM default protocol.
 * Returns number of successful written bytes or 0 on error.
 */
static uint16_t 
ieee_raw_write(uint16_t len, uint8_t flags)
{
    uint8_t atn, talk, data, device, sa;
    uint16_t rv;

    rv = len;
    atn = flags & XUM_WRITE_ATN;
    talk = flags & XUM_WRITE_TALK;

    eoi = 0;
    ieee_status = 0;

    usbInitIo(len, ENDPOINT_DIR_OUT);

    if(atn && len >= 1)
    {
        // get device# from USB 
        if (usbRecvByte(&device) != 0) 
        {
            return 0;
        }
        len--;
        if(len == 0 || ((device & 0x1f) == 0x1f))
        {
            // unlisten, untalk
            if (device & 0x40) 
            {
                IeeeUntalk();
            }
            if (device & 0x20) 
            {
                IeeeUnlisten();
            }
        }
        else
        {
            // get secondary-address from USB
            if (usbRecvByte(&sa) != 0) 
            {
                return 0;
            }
            len--;

            if (talk) 
            {
                IeeeTalk(device, sa);
                len = 0;
            }
            else if(len > 2)
            {
                // open
            }
            else switch(sa & 0xf0)
            {
              case 0xE0:
                // close
                IeeeClose(device, sa);
                break;
              case 0xF0:
                // open
                IeeeOpen(device, sa, NULL);
                break;
              case 0x60:
                // listen
                IeeeListen(device, sa);
                break;
              default:
                // error????
                break;
            }
        }
    }


    // send data
    //
    while (len != 0) {
        // Get a data byte from host, quitting if it signalled an abort.
        if (usbRecvByte(&data) != 0) 
        {
            rv = 0;
            break;
        }
        if (IeeeBsout(data)) 
        {
            rv = 0;
            break;
        } 
        len--;

        // watchdog
        wdt_reset();
    }
    usbIoDone();

    return rv;
}

/* 
 * Reads bytes from the drive via the CBM default protocol.
 * Returns number of successful written bytes or 0 on error.
 */
static uint16_t
ieee_raw_read(uint16_t len)
{
    uint8_t     by;
    uint16_t to, count;

    usbInitIo(len, ENDPOINT_DIR_IN);

    count = 0;
    do {
        // read again after EOI??
        if (eoi) {
            usbIoDone();
            return 0;
        }

        to = 0;
        while(1)
        {
            ieee_status &= ~IEEE_ST_RDTO;
            by = IeeeBasin();
            if(!(ieee_status & IEEE_ST_RDTO))
                break;

            if(to >= 20 || !TimerWorker()) 
            {
                /* 1.3 (20 * 65ms) sec timeout */
                usbIoDone();
                return 0;
            }
            to++;
        }

 
        // Send the data byte to host, quitting if it signalled an abort.
        if (usbSendByte(by))
            break;

        count++;
        wdt_reset();
    } while (count != len && !eoi);

    usbIoDone();
    return count;
}

//----------------------------------------------------------------------
// IEEE SEND BYTE 
static int8_t IeeeByteOut(uint8_t by)
{
    int8_t     rc=0;

    IeeeDav(1);

    if(IEEE_NRFD && IEEE_NDAC) 
    {
        ieee_status |= IEEE_ST_DNP;                    // !!DEVICE NOT PRESENT
        return 1;            
    }

    while(!IEEE_NRFD)                                // WAIT WHILE NRFD
        wdt_reset();                                    // watchdog

    IeeeDataOut(~by);                                // OUTPUT!
    _delay_us(5);
    IeeeDav(0);

    ieee_timer = 65;                                // 65ms timeout

    while(!IEEE_NDAC)                                // WAIT FOR DAC
    {
        if(IsTimeout())
        {
            ieee_status |= IEEE_ST_WRTO;            // 65ms write timeout
            rc = 1;
            break;
        }
    }

    IeeeDav(1);
    IeeeDataIn();                                    // DATAPORT INPUT!

    return rc;
}

//----------------------------------------------------------------------
// IEEE GET BYTE 
static uint8_t IeeeIn(void)
{
    uint8_t     rc=0;

#ifdef DEBUG1
    uartPrintf_p("IeeeIn()");
    uartPutCrLf();
#endif

    IeeeNdac(0);                    // NDAC low
    IeeeNrfd(1);                    // ready for data!
    
    ieee_timer = 65;                // 65ms timeout

    while(1)                        // WAIT FOR DAV
    {
        if(!IEEE_DAV) 
        {
            break;
        }
        if(IsTimeout())
        {
            ieee_status |= IEEE_ST_RDTO;    // read timeout

            IeeeNdac(0);            // NDAC low
            IeeeNrfd(0);            // NRFD low
            return 0;
        }
    }
    _delay_us(1);
    rc = ~IEEE_DATA;
    if(!IEEE_EOI)
    {
        ieee_status |= IEEE_ST_EOI;    // last byte - EOI
        eoi = 1;                    // for XUM1541_IOCTL
    }

    IeeeNrfd(0);                    // NRFD low
    _delay_us(1);
    IeeeNdac(1);                    // data accepted!

    while(!IEEE_DAV)                // WAIT FOR DAV HIGH
        wdt_reset();

    IeeeNdac(0);                    // NDAC low
    return rc;
}

//----------------------------------------------------------------------
// IEEE SEND ATN
static int8_t IeeeAtnOut(uint8_t by)
{
    while(1) 
    {
        IeeeNrfd(1);
        IeeeNdac(1);

        if(last_byte >= 0)
        {
            // SEND LAST BYTE WITH EOI
            IeeeEoi(0);
            IeeeByteOut(last_byte);
            IeeeEoi(1);

            last_byte = -1;
        }
        else
        {
            ieee_timer = 255;                            // 255ms timeout

            while(!IEEE_DAV)                            // WAIT WHILE DAV
            {
                if(IsTimeout())
                {
                    ieee_status |= IEEE_ST_ATTO;        // ATN-OUT timeout
                    return 1;
                }
            }
        }

        IeeeAtn(0);                                        // SET ATN
        _delay_us(ATN_DELAY);

        if(IeeeByteOut(by)) break;

        //
        // ATN ok, device selected ==> next send SA
        //
        return 0;
    }

    //
    // abort
    //
    IeeeAtn(1);                                            // ATN hi
    return 1;
}

//----------------------------------------------------------------------
// IEEE SEND LISTEN SECONDARY ADDRESS
static int8_t IeeeSecListen(uint8_t sa)
{
    int8_t    rc;

    if((rc = IeeeByteOut(sa)) == 0)
        ieee_listen = true;

    IeeeAtn(1);                                            // SET ATN HI
    return rc;
}

//----------------------------------------------------------------------
// IEEE SEND TALK SECONDARY ADDRESS
static int8_t IeeeSecTalk(uint8_t sa)
{
    int8_t    rc;

    if(!(rc = IeeeByteOut(sa)))
    {
        IeeeNrfd(0);
        IeeeNdac(0);
        ieee_talk    = true;
    }
    IeeeAtn(1);                                    // SET ATN HI
    return rc;
}

//----------------------------------------------------------------------
// IEEE SEND BYTE 
static int8_t IeeeBsout(uint8_t by)
{
    if(last_byte >= 0)
    {
        // SEND BYTE IN BUFFER
        if(IeeeByteOut(last_byte)) 
            return 1;
    }
    last_byte = by;
    return 0;
}

//----------------------------------------------------------------------
// IEEE GET BYTE 
static uint8_t IeeeBasin()
{
    if(!(eoi))
    {
        return IeeeIn();
    }
    return 0;
}

//----------------------------------------------------------------------
// UNTALK
static int8_t IeeeUntalk(void)
{
    int8_t     rc;

    rc = IeeeAtnOut(0x5f);
    IeeeAtn(1);                                    // SET ATN HI
    _delay_us(ATN_DELAY);

    ieee_talk    = false;
    return rc;
}

//----------------------------------------------------------------------
// UNLISTEN
static int8_t IeeeUnlisten(void)
{
    int8_t     rc;

    rc = IeeeAtnOut(0x3f);
    IeeeAtn(1);                                    // SET ATN HI
    _delay_us(ATN_DELAY);

    ieee_listen    = false;
    return rc;
}

//----------------------------------------------------------------------
// LISTEN
static int8_t IeeeListen(uint8_t dev, uint8_t sa)
{
    int8_t     rc=0;

    if(IeeeAtnOut(0x20 | (dev & 0x1f)))
        rc = 1;
    else if(IeeeSecListen(0x60 | (sa & 0x1f)))
        rc = 1;
    return rc;
}

//----------------------------------------------------------------------
// TALK
static int8_t IeeeTalk(uint8_t dev, uint8_t sa)
{
    int8_t     rc=0;

    //debug_LED_blink(1);

    if(IeeeAtnOut(0x40 | (dev & 0x1f)))
        rc = 1;
    else if(IeeeSecTalk(0x60 | (sa & 0x0f)))
        rc = 1;
    //else
    //    debug_LED_blink(3);
    return rc;
}

//----------------------------------------------------------------------
// OPEN
static int8_t IeeeOpen(uint8_t dev, uint8_t sa, char *s)
{
    int8_t     rc=0;

    if(IeeeAtnOut(0x20 | (dev & 0x1f)))
        rc = 1;
    else if(IeeeSecListen(0xf0 | (sa & 0x0f)))
        rc = 1;
    else 
    {
        //debug_LED_blink(1);
        if(s != NULL)
        {
            while(*s)
            {
                if(IeeeBsout(*s)) 
                {
                    rc = 1;
                    break;
                }
                ++s;
                //debug_LED_blink(1);
                IeeeUnlisten();
            }
        }
    }
    return rc;
}

//----------------------------------------------------------------------
// CLOSE
static int8_t IeeeClose(uint8_t dev, uint8_t sa)
{
    int8_t     rc=0;

    if(IeeeAtnOut(0x20 | (dev & 0x1f)))                // LISTEN
        rc = 1;
    else if(IeeeSecListen(0xe0 | (sa & 0x0f)))        // CLOSE SA
        rc = 1;

    IeeeUnlisten();
    return rc;
}
