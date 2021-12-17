/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2004, 2007 Spiro Trikaliotis
 *
 */

/*! **************************************************************
** \file sys/libiec/i_iec.h \n
** \author Spiro Trikaliotis \n
** \n
** \brief Internal functions and definitions of the libiec library
**
****************************************************************/

#ifndef I_CBMIEC_H
#define I_CBMIEC_H

#include "iec.h"

/* The port addresses (relative) of the parallel port */

/*! the DATA register is located here */
#define PARALLEL_DATA_OFFSET 0
/*! the STATUS register is located here */
#define PARALLEL_STATUS_OFFSET 1
/*! the CONTROL register is located here */
#define PARALLEL_CONTROL_OFFSET 2
/*! the count of port addresses the parallel port occupies */
#define PARALLEL_REGISTER_SPAN 3


/* lpt output lines */
/* this is correct for a XA1541/XM1541; a XE1541 has DATA and RESET exchanged! */

#define PP_ATN_OUT    (Pdx->IecAtnOut)   //!< The ATN OUT bit
#define PP_CLK_OUT    (Pdx->IecClkOut)   //!< The CLOCK OUT bit
#define PP_DATA_OUT   (Pdx->IecDataOut)  //!< The DATA OUT bit
#define PP_RESET_OUT  (Pdx->IecResetOut) //!< The RESET OUT bit

/* additional LP control */
#define PP_LP_IRQ     0x10            //!< Bit for allowing interrupts of the LPT
#define PP_LP_BIDIR   0x20            //!< Bit for setting set bidirectional mode of the LPT

/* lpt input lines */
#define PP_ATN_IN     (Pdx->IecAtnIn)   //!< The ATN IN bit
#define PP_CLK_IN     (Pdx->IecClkIn)   //!< The CLOCK IN bit
#define PP_DATA_IN    (Pdx->IecDataIn)  //!< The DATA IN bit
#define PP_RESET_IN   (Pdx->IecResetIn) //!< The RESET IN bit

/*! Get the address of the parallel port DATA register out of the Pdx info */
#define PAR_PORT (Pdx->ParPortPortAddress + PARALLEL_DATA_OFFSET)
/*! Get the address of the parallel port STATUS register (= the port for input)
 * out of the Pdx info */
#define IN_PORT  (Pdx->ParPortPortAddress + PARALLEL_STATUS_OFFSET)
/*! Get the address of the parallel port CONTROL register (= the port for
 * output) out of the Pdx info */
#define OUT_PORT (Pdx->ParPortPortAddress + PARALLEL_CONTROL_OFFSET)

/*! set an output line on the parallel port */
#define CBMIEC_SET(_set)              do { DBG_ASSERT((_set) != 0); Pdx->IecOutBits|=(_set); WRITE_PORT_UCHAR(OUT_PORT,(UCHAR)(Pdx->IecOutEor ^ Pdx->IecOutBits)); } while (0)
/*! release an output line on the parallel port */
#define CBMIEC_RELEASE(_rel)          do { DBG_ASSERT((_rel) != 0); Pdx->IecOutBits&=~(_rel); WRITE_PORT_UCHAR(OUT_PORT,(UCHAR)(Pdx->IecOutEor ^ Pdx->IecOutBits)); } while (0)
/*! set and release an output line on the parallel port (simultaneously) */
#define CBMIEC_SET_RELEASE(_set,_rel) do { DBG_ASSERT((_set) != 0); DBG_ASSERT((_rel) != 0); Pdx->IecOutBits|=(_set); Pdx->IecOutBits&=~(_rel); \
                                      WRITE_PORT_UCHAR(OUT_PORT,(UCHAR)(Pdx->IecOutEor ^ Pdx->IecOutBits)); } while (0)

/*! for the "hibernate workaround": Find out if the status of the control lines is like it should be */
#define CBMIEC_ARE_OUTPUT_LINES_CORRECT() ( READ_PORT_UCHAR(OUT_PORT) == (UCHAR)(Pdx->IecOutEor ^ Pdx->IecOutBits) )

/*! get the value of the parallel port */
#define CBMIEC_GET(_line)             (((READ_PORT_UCHAR(IN_PORT) ^ Pdx->IecInEor) & _line)==0?1:0)


/*! The various timeouts of the IEC bus protocol */
typedef
struct IEC_TIMEOUTS
{
    //! = 100 us: How long is a RESET being held?
    ULONG T_holdreset;
    //! = 5 s: How long to delay after a RESET
    ULONG T_afterreset;

    //! = 10 us: Graunularity of wait_for_listener() polls
    ULONG T_WaitForListener_Granu_T_H;

    //! = 20 us: Starting reception, granularity for the wait until CLK is low
    ULONG T_1_RECV_WAIT_CLK_LOW_DATA_READY_GRANU;

    //! x T_2 is 40
    ULONG T_2_Times;

    //! = 10 us: Granularity: How long do we wait for the data of the other site
    ULONG T_2_RECV_WAIT_CLK_HIGH_T_NE;

    //! = 70 us: How long to set DATA to ack an EOI
    ULONG T_3_RECV_EOI_RECOGNIZED;

    //! x T_4, is 100: Wait for CLK high after an EOI
    ULONG T_4_Times;

    //! = 20 us: Granularity: Wait for CLK high after an EOI
    ULONG T_4_RECV_WAIT_CLK_HIGH_AFTER_EOI_GRANU;

    //! x T_5, is 200: How long to wait for CLK high (=active) for every single bit
    ULONG T_5_Times;

    //! = 10 us: Granularity: How long to wait for CLK high (=active) for every single bit
    ULONG T_5_RECV_BIT_WAIT_CLK_HIGH;

    //! x T_6, is 100: How long to wait for CLK low again (=inactive) after every single bit
    ULONG T_6_Times;

    //! = 20 us: How long to wait for CLK low again (=inactive) after every single bit
    ULONG T_6_RECV_BIT_WAIT_CLK_LOW;

    //! = 70 us: Inter-byte delay on reception
    ULONG T_7_RECV_INTER_BYTE_DELAY;

    //! = 20 us: For cbmiec_iec_wait(): Granularity (long)
    ULONG T_8_IEC_WAIT_LONG_DELAY;

    //! = 10 us: For cbmiec_iec_wait(): Granularity (short)
    ULONG T_8_IEC_WAIT_SHORT_DELAY;

    //! x T_9a, is 100: On send, how long to wait for driver to set DATA
    ULONG T_9_Times;

    //! = 10 us: Granularity: On send, how long to wait for driver to set DATA
    ULONG T_9_SEND_WAIT_DEVICES_T_AT;

    //! = 20 us: Extra wait before sending 1st byte
    ULONG T_10_SEND_BEFORE_1ST_BYTE;

    //! = 50 us: Extra wait before sending of every single byte
    ULONG T_11_SEND_BEFORE_BYTE_DELAY;

    //! = 100 us: Extra wait after sending every single byte
    ULONG T_12_SEND_AFTER_BYTE_DELAY;

    //! = 20 us: On listener-talker-turnaround, how long to wait until CLK is released
    ULONG T_13_SEND_TURN_AROUND_LISTENER_TALKER_T_TK;

    //! = 100 us: Extra wait after sending a block
    ULONG T_14_SEND_AT_END_DELAY;

    // sendbyte related:

    //! = 70 us: Inter-bit wait time while sending a byte
    ULONG T_15_SEND_BEFORE_BIT_DELAY_T_S;

    //! = 20 us: How long to hold CLK low for every bit while sending
    ULONG T_16_SEND_BIT_TIME_T_V;

    //! x T_17, is 20: How long to wait for a frame handshake after sending a byte
    ULONG T_17_Times;

    //! = 100 us: Granularity: How long to wait for a frame handshake after sending a byte
    ULONG T_17_SEND_FRAME_HANDSHAKE_T_F;

    //! = 300ms, timeout for reading one handshaked byte with parallel burst
    ULONG T_PARALLEL_BURST_READ_BYTE_HANDSHAKED;

    //! = 300ms, timeout for writing one handshaked byte with parallel burst
    ULONG T_PARALLEL_BURST_WRITE_BYTE_HANDSHAKED;

} IEC_TIMEOUTS;

/*! timeout values */
extern IEC_TIMEOUTS libiec_global_timeouts;

#if DBG

/* The functions DbgWp() and DbgRp() are used to intercept
 * NDIS_WRITE_PORT() and NDIS_READ_PORT() when debugging.
 */
extern VOID
DbgWp(IN PUCHAR Port, IN UCHAR Value);

extern UCHAR
DbgRp(IN PUCHAR Port);

/*! \brief show the value of parallel port lines

 cbmiec_show_port() is used to show the value of the
 parallel port lines if needed for debugging

 \param s
    @@@@@ \todo document
 */
extern VOID
cbmiec_show_port(UCHAR *s);

/*
 * If DBG is defined, we want to be able to monitor every
 * READ_PORT_UCHAR and WRITE_PORT_UCHAR, so we override them
 * here with our own functions:
 */

/*! \brief READ_PORT_UCHAR replacement for debugging

 \param _x_
    the port number to read

 \return
    The current value on that port
*/
#define READ_PORT_UCHAR(_x_)       DbgRp(_x_)

/*! \brief WRITE_PORT_UCHAR replacement for debugging

 \param _x_
    the port number to read

 \param _y_
    the value to put into that port number
*/
#define WRITE_PORT_UCHAR(_x_, _y_) DbgWp(_x_, _y_)

/*
 *  dump input lines
 */
extern VOID
cbmiec_show_state(IN PDEVICE_EXTENSION Pdx, IN UCHAR *Str);

#else /* #if DBG */

    //! A cbmiec_show_state() implementation for release builds
    #define cbmiec_show_state(_x_, _y_)

#endif /* #if DBG */

extern VOID
cbmiec_schedule_timeout(IN ULONG howlong); // howlong in us!

extern VOID
cbmiec_udelay(IN ULONG howlong); // howlong in ms!

extern NTSTATUS
cbmiec_i_raw_read(IN PDEVICE_EXTENSION Pdx, OUT UCHAR *buf, ULONG cnt, OUT ULONG *pReceived);

extern NTSTATUS
cbmiec_i_raw_write(PDEVICE_EXTENSION Pdx, const UCHAR *buf, ULONG cnt, ULONG *pSent, BOOLEAN atn, BOOLEAN talk);

extern VOID
cbmiec_block_irq(PDEVICE_EXTENSION Pdx);

extern VOID
cbmiec_release_irq(PDEVICE_EXTENSION Pdx);

extern LONG
cbmiec_i_pp_read_debounced(IN PDEVICE_EXTENSION Pdx);


/*! \brief remember in which state the cable is currently */

typedef
enum cablestate_e
{
    CABLESTATE_UNKNOWN,            /*!< not tested yet */
    CABLESTATE_ERROR_OCCURRED,     /*!< cable has been tested, but a transfer ended with an error */
    CABLESTATE_TESTED,             /*!< tested, but not used yet */
    CABLESTATE_SUCCESSFULLY_USED   /*!< tested and successfully used */
} CABLESTATE;

extern VOID
cbmiec_setcablestate(PDEVICE_EXTENSION Pdx, CABLESTATE State);

#ifdef USE_DPC

    extern VOID
    cbmiec_dpc(IN PKDPC Dpc, IN PDEVICE_OBJECT Fdo, IN PIRP Irp, IN PVOID Context);

#endif // #ifdef USE_DPC

#endif /* #ifndef I_CBMIEC_H */
