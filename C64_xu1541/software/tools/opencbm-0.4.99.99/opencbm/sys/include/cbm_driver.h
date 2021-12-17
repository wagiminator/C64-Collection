/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2004 Spiro Trikaliotis
 *
 */

/*! **************************************************************
** \file sys/include/cbm_driver.h \n
** \author Spiro Trikaliotis \n
** \n
** \brief Definitions for the opencbm driver
**
****************************************************************/

/*! \brief @@@@@ \todo document */
typedef struct DEVICE_EXTENSION_s DEVICE_EXTENSION;

/*! \brief @@@@@ \todo document */
typedef struct DEVICE_EXTENSION_s * PDEVICE_EXTENSION;

#include "arch_cbm_driver.h"

/*! Debugging is for the kernel-mode */
#define DBG_KERNELMODE

#include "debug.h"

#include "perfeval.h"

/* The following defines are used if performance evaluation is selected. */

/*! Performance evaluation: Make sure the measuring system works as expected */
#define PERF_EVENT_MEASURE_TIME(_x_)    PERF_EVENT(0x1, (ULONG_PTR) _x_)

/*! Performance evaluation: StartIo() is performed */
#define PERF_EVENT_STARTIO(_x_)         PERF_EVENT(0x10, (ULONG_PTR) _x_)
/*! Performance evaluation: An IRP is completed */
#define PERF_EVENT_COMPLETEIRP(_x_)     PERF_EVENT(0x11, (ULONG_PTR) _x_)
/*! Performance evaluation: An IRP is cancelled */
#define PERF_EVENT_CANCELIRP(_x_)       PERF_EVENT(0x12, (ULONG_PTR) _x_)

/*! Performance evaluation: An IOCTL is queued */
#define PERF_EVENT_IOCTL_QUEUE(_x_)     PERF_EVENT(0x20, _x_)
/*! Performance evaluation: An IOCTL is executed */
#define PERF_EVENT_IOCTL_EXECUTE(_x_)   PERF_EVENT(0x21, _x_)
/*! Performance evaluation: An OPEN request is queued */
#define PERF_EVENT_OPEN_QUEUE()         PERF_EVENT(0x30, 0)
/*! Performance evaluation: An OPEN request is executed */
#define PERF_EVENT_OPEN_EXECUTE()       PERF_EVENT(0x31, 0)
/*! Performance evaluation: An CLOSE request is queued */
#define PERF_EVENT_CLOSE_QUEUE()        PERF_EVENT(0x40, 0)
/*! Performance evaluation: An CLOSE request is executed */
#define PERF_EVENT_CLOSE_EXECUTE()      PERF_EVENT(0x41, 0)
/*! Performance evaluation: A READ request is queued */
#define PERF_EVENT_READ_QUEUE(_x_)      PERF_EVENT(0x50, _x_)
/*! Performance evaluation: A READ request is executed */
#define PERF_EVENT_READ_EXECUTE(_x_)    PERF_EVENT(0x51, _x_)
/*! Performance evaluation: A WRITE request is queued */
#define PERF_EVENT_WRITE_QUEUE(_x_)     PERF_EVENT(0x60, _x_)
/*! Performance evaluation: A WRITE request is executed */
#define PERF_EVENT_WRITE_EXECUTE(_x_)   PERF_EVENT(0x61, _x_)

/*! Performance evaluation: Schedule a start of the thread */
#define PERF_EVENT_THREAD_START_SCHED() PERF_EVENT(0x100, 0)
/*! Performance evaluation: Thread is actually started */
#define PERF_EVENT_THREAD_START_EXEC()  PERF_EVENT(0x100, 1)
/*! Performance evaluation: Thread polls for IRPs */
#define PERF_EVENT_THREAD_POLL()        PERF_EVENT(0x100, 2)
/*! Performance evaluation: Schedule a stop of the thread */
#define PERF_EVENT_THREAD_STOP_SCHED()  PERF_EVENT(0x100, 3)
/*! Performance evaluation: Thread is actually stopped */
#define PERF_EVENT_THREAD_STOP_EXEC()   PERF_EVENT(0x100, 4)

/*! Performance evaluation: Reading byte no _x_ */
#define PERF_EVENT_READ_BYTE_NO(_x_)    PERF_EVENT(0x200, _x_)
/*! Performance evaluation: Read a byte _x_ */
#define PERF_EVENT_READ_BYTE(_x_)       PERF_EVENT(0x201, _x_)
/*! Performance evaluation: Reading bit no. _x_ */
#define PERF_EVENT_READ_BIT_NO(_x_)     PERF_EVENT(0x202, _x_)
/*! Performance evaluation: Writing byte no _x_ */
#define PERF_EVENT_WRITE_BYTE_NO(_x_)   PERF_EVENT(0x210, _x_)
/*! Performance evaluation: Writing byte _x_ */
#define PERF_EVENT_WRITE_BYTE(_x_)      PERF_EVENT(0x211, _x_)
/*! Performance evaluation: Writing bit no. _x_ */
#define PERF_EVENT_WRITE_BIT_NO(_x_)    PERF_EVENT(0x212, _x_)

/*! \brief @@@@@ \todo document */
typedef enum cablestate_e CABLESTATE;

/*! The device extension for the device */
typedef
struct DEVICE_EXTENSION_s {

    /*! the architecture specific stuff */
    ARCH_DEVICE_EXTENSION;

    /*! Start address of the parallel port in the I/O address space */
    PUCHAR ParPortPortAddress;

    /*! Start address of the ECP portion of the parallel port.
        If this is not an ECP port, this is 0.
     */
    PUCHAR ParPortEcpPortAddress;

    /*! how many Irqs should we wait in cbmiec_interrupt
     * (for cbmiec_wait_for_listener()? This determines
     * if we want to schedule an EOI (=2) or not (=1).
     * =0 means that the interrupt is not interesting for us.
     */
    ULONG IrqCount;

    /*! FLAG: We are running on an SMP or an HT machine */
    BOOLEAN IsSMP;

    // IEC related vars:

    /*!
     * The type of the cable, as determined by the automatism:
     * IEC_CABLETYPE_AUTO --> autodetect
     * IEC_CABLETYPE_XM   --> non-inverted (XM1541)
     * IEC_CABLETYPE_XA   --> inverted (XA1541)
     */
    IEC_CABLETYPE IecCable;

    /*!
     * The type of the cable, as set by the user
     */
    IEC_CABLETYPE IecCableUserSet;

    /*!
     * The state the cable detection is in
     */
    CABLESTATE IecCableState;

    /*! The current state of the output bits of the parallel port */
    UCHAR IecOutBits;

    /*! EOR mask for outputting on the parallel port */
    UCHAR IecOutEor;

    /*! EOR mask for inputting from the parallel port */
    UCHAR IecInEor;

    /* The values below tell the drive which mask has to be used
     * to set and/or release a line
     */

    /* lpt output lines */

    UCHAR IecAtnOut;   //!< The ATN OUT bit
    UCHAR IecClkOut;   //!< The CLOCK OUT bit
    UCHAR IecDataOut;  //!< The DATA OUT bit
    UCHAR IecResetOut; //!< The RESET OUT bit

    /* lpt input lines */

    UCHAR IecAtnIn;   //!< The ATN IN bit
    UCHAR IecClkIn;   //!< The CLOCK IN bit
    UCHAR IecDataIn;  //!< The DATA IN bit
    UCHAR IecResetIn; //!< The RESET IN bit


    /*! Remember if this device is busy */
    BOOLEAN IecBusy;

    /*! TRUE -> we received an EOI over the bus */
    BOOLEAN Eoi;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;
