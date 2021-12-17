/*! **************************************************************
** \file include/WINDOWS/cbmioctl.h \n
** \author Spiro Trikaliotis \n
** \authors Based on code from
**    Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
** \n
** \brief Define the IOCTL codes for the opencbm driver
**
****************************************************************/

/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Copyright 2001-2006,2008-2009 Spiro Trikaliotis
 *  Copyright 2009      Arnd <arnd(at)jonnz(dot)de>
*/

#ifndef CBM_IOCTL_H
#define CBM_IOCTL_H

/*! Tell the type of the cable used */
typedef
enum iec_cabletype
{
    IEC_CABLETYPE_UNSPEC = -2, /*< don't use! */
    IEC_CABLETYPE_AUTO = -1, IEC_CABLETYPE_XM = 0, IEC_CABLETYPE_XA = 1,
    IEC_CABLETYPE_XE
} IEC_CABLETYPE;



/*! Registry key (under HKLM) of the service
 */
#define CBM_REGKEY_SERVICE "System\\CurrentControlSet\\Services\\opencbm"

/*! Registry entry (under CBM_REGKEY_SERVICE) of the default LPT port
 */
#define CBM_REGKEY_SERVICE_DEFAULTLPT "DefaultLpt"

/*! Registry entry (under CBM_REGKEY_SERVICE) for the cable type
 */
#define CBM_REGKEY_SERVICE_IECCABLE "CableType"

/*! Registry entry (under CBM_REGKEY_SERVICE) for determining if the parport is to be permanently locked
 */
#define CBM_REGKEY_SERVICE_PERMLOCK "PermanentlyLock"

/*! Registry entry (under CBM_REGKEY_SERVICE) of the DebugFlags */
#define CBM_REGKEY_SERVICE_DEBUGFLAGS "DebugFlags"

/*! Registry entry (under CBM_REGKEY_SERVICE) of the DebugFlags for the DLL */
#define CBM_REGKEY_SERVICE_DLL_DEBUGFLAGS "DllDebugFlags"

/*! The name of the driver with which to communicate */
#define OPENCBM_DRIVERNAME "opencbm"

/*! The device name of the driver
 * This is necessary for DOS drivers and "old-style" DLLs
 * A number is appended to this name, starting with 0.
 */
#define CBMDEVICENAME_MAINPART "\\\\.\\" OPENCBM_DRIVERNAME

/*! the GUID for communication with the kernel-mode driver
 \todo Currently, this is unused! */

// {9C3B5B5E-558E-47cc-9C05-F1FCF5175407}
DEFINE_GUID(GUID_OPENCBM, 0x9c3b5b5e, 0x558e, 0x47cc, 0x9c, 0x5, 0xf1, 0xfc, 0xf5, 0x17, 0x54, 0x7);

/*! CBMT_LINE: DATA  */
#define IEC_LINE_DATA   0x01
/*! CBMT_LINE: CLOCK  */
#define IEC_LINE_CLOCK  0x02
/*! CBMT_LINE: ATN  */
#define IEC_LINE_ATN    0x04
/*! CBMT_LINE: RESET  */
#define IEC_LINE_RESET  0x08


// Data structures for accessing the kernel mode driver:

/*! This type provides primary and secondary address on the IEC bus */
typedef
struct CBMT_IECADDRESS
{
    /*! The primary address a.k.a. device number of the drive to be accessed */
    UCHAR PrimaryAddress;

    /*! The secondary address for the access */
    UCHAR SecondaryAddress;

} CBMT_IECADDRESS;

/*! This type provides a single (raw) byte */
typedef
struct CBMT_SINGLEBYTE
{
    /*! Just a single byte */
    UCHAR Byte;

} CBMT_SINGLEBYTE;

/*! This type gives a line (one out of IEC_ATN, IEC_DATA, IEC_CLOCK) */
typedef
struct CBMT_LINE
{
    /*! The line to be accessed. Must be one of IEC_LINE_RESET, IEC_LINE_ATN,
     * IEC_LINE_DATA, IEC_LINE_CLOCK */
    UCHAR Line;

} CBMT_LINE;

/*! This type gives a line (one out of IEC_ATN, IEC_DATA, IEC_CLOCK)
    and the corresponding state.
 */
typedef
struct CBMT_LINESTATE
{
    /*! The line to be accessed. Must be one of IEC_LINE_RESET, IEC_LINE_ATN,
     * IEC_LINE_DATA, IEC_LINE_CLOCK */
    UCHAR Line;

    /*! The state for which the line is to be tested */
    UCHAR State;

} CBMT_LINE_STATE;

/*! This type contains a boolean decision */
typedef
struct CBMT_BOOLEAN
{
    /*! The decision which is taken */
    BOOLEAN Decision;

} CBMT_BOOLEAN;

/*! These macros define how to extract version information
 * from CBMT_I_INSTALL_OUT.DriverVersion and/or .DllVersion
 * Assume that a version is x.y.z.w
 */

/*! the major number, that is, x */
#define CBMT_I_INSTALL_OUT_GET_VERSION_MAJOR(_x)    (((_x) >> 24) & 0xFF)
/*! the minor number, that is, y */
#define CBMT_I_INSTALL_OUT_GET_VERSION_MINOR(_x)    (((_x) >> 16) & 0xFF)
/*! the subminor number, that is, z */
#define CBMT_I_INSTALL_OUT_GET_VERSION_SUBMINOR(_x) (((_x) >>  8) & 0xFF)
/*! the devel number, that is, w */
#define CBMT_I_INSTALL_OUT_GET_VERSION_DEVEL(_x)    (((_x) >>  0) & 0xFF)

/*! Build such a version number */
#define CBMT_I_INSTALL_OUT_MAKE_VERSION(_x, _y, _z, _w) \
    (((((_x) << 8 | (_y)) << 8 | (_z)) << 8 | (_w)))


/*! the bugfix number in the version extension */
#define CBMT_I_INSTALL_OUT_GET_VERSION_EX_BUGFIX(_x) (((_x) >>  0) & 0xFF)

/*! Build such a version extension */
#define CBMT_I_INSTALL_OUT_MAKE_VERSION_EX(_w) \
    (_w)

/*! Output buffer for I_INSTALL */
typedef
struct CBMT_I_INSTALL_OUT
{
    /*! Flags: Is the installation correct, or are there errors?
     * cf. CBM_I_DRIVER_INSTALL_xxx constants
     */
    ULONG ErrorFlags;

    /*! The version of the DLL */
    ULONG DllVersion;

    /*! The version of the driver */
    ULONG DriverVersion;

    /*! The version extension of the DLL */
    ULONG DllVersionEx;

    /*! The version extension of the driver */
    ULONG DriverVersionEx;

} CBMT_I_INSTALL_OUT, *PCBMT_I_INSTALL_OUT;

/*! Output buffer for CBMCTRL_TEST_IRQ.
 * Values of 1 indicate an error, values of
 * -1 are warnings only.
 */
typedef
struct CBMT_I_TESTIRQ
{
    /*! Was there an error in acquiring the interrupt
        from the driver? */
    CHAR ErrAcquireIrq;

    /*! Was there an error while trying to generate an interrupt
        on the rising edge? */
    CHAR ErrIrqRisingEdge;

    /*! Was there an error while trying to generate an interrupt
        on the falling edge? */
    CHAR ErrIrqFallingEdge;

} CBMT_I_TESTIRQ, *PCBMT_I_TESTIRQ;

/*! A value for both the CBMT_IEC_DBG_READ and CBMT_IEC_DBG_WRITE IOCTLs */
typedef
struct CBMT_IEC_DBG_VALUE
{
    UCHAR Value;   /*!< some value. This is used in different ways */
} CBMT_IEC_DBG_VALUE;

/*! Input buffer for TALK */
typedef CBMT_IECADDRESS CBMT_TALK_IN;
/*! Input buffer for LISTEN */
typedef CBMT_IECADDRESS CBMT_LISTEN_IN;
/*! Input buffer for OPEN */
typedef CBMT_IECADDRESS CBMT_OPEN_IN;
/*! Input buffer for CLOSE */
typedef CBMT_IECADDRESS CBMT_CLOSE_IN;
/*! Input buffer for IEC_PP_WRITE */
typedef CBMT_SINGLEBYTE CBMT_PP_WRITE_IN;
/*! Output buffer for IEC_PP_READ */
typedef CBMT_SINGLEBYTE CBMT_PP_READ_OUT;
/*! Output buffer for IEC_POLL */
typedef CBMT_LINE       CBMT_IEC_POLL_OUT;
/*! Input buffer for IEC_WAIT */
typedef CBMT_LINE_STATE CBMT_IEC_WAIT_IN;
/*! Output buffer for IEC_WAIT */
typedef CBMT_LINE       CBMT_IEC_WAIT_OUT;
/*! Input buffer for IEC_SET */
typedef CBMT_LINE       CBMT_IEC_SET_IN;
/*! Input buffer for RELEASE */
typedef CBMT_LINE       CBMT_IEC_RELEASE_IN;
/*! Output buffer for GET_EOI */
typedef CBMT_BOOLEAN    CBMT_GET_EOI_OUT;
/*! Input buffer for IEC_SET */
typedef CBMT_LINE_STATE CBMT_IEC_SETRELEASE_IN;
/*! Output buffer for IEC_PARBURST_READ */
typedef CBMT_SINGLEBYTE CBMT_PARBURST_PREAD_OUT;
/*! Input buffer for IEC_PARBURST_PAR_WRITE */
typedef CBMT_SINGLEBYTE CBMT_PARBURST_PWRITE_IN;
/*! Output buffer for IEC_DBG_READ */
typedef CBMT_IEC_DBG_VALUE CBMT_IEC_DBG_READ;
/*! Input buffer for IEC_DBG_WRITE */
typedef CBMT_IEC_DBG_VALUE CBMT_IEC_DBG_WRITE;


/*! BASE number of the custom IOCTL */
#define CBMCTRL_BASE    0x0000A424
/*! INDEX number of the custom IOCTLs */
#define OPENCBM_IOCTL_INDEX  0x823

/*! Define a IOCTL */
#define _CBMIO(_a,_b) CTL_CODE(CBMCTRL_BASE, (OPENCBM_IOCTL_INDEX+(_b)), \
                   METHOD_BUFFERED, FILE_ANY_ACCESS)

                                                  // INPUT:               OUTPUT:
//! IOCTL for sending a TALK command
#define CBMCTRL_TALK        _CBMIO(CBMCTRL_BASE, 0)  // CBMT_TALK_IN         -
//! IOCTL for sending a LISTEN command
#define CBMCTRL_LISTEN      _CBMIO(CBMCTRL_BASE, 1)  // CBMT_LISTEN_IN       -
//! IOCTL for sending an UNTALK command
#define CBMCTRL_UNTALK      _CBMIO(CBMCTRL_BASE, 2)  // -                    -
//! IOCTL for sending an UNLISTEN command
#define CBMCTRL_UNLISTEN    _CBMIO(CBMCTRL_BASE, 3)  // -                    -
//! IOCTL for sending an OPEN command
#define CBMCTRL_OPEN        _CBMIO(CBMCTRL_BASE, 4)  // CBMT_OPEN_IN         -
//! IOCTL for sending a CLOSE command
#define CBMCTRL_CLOSE       _CBMIO(CBMCTRL_BASE, 5)  // CBMT_CLOSE_IN        -
//! IOCTL for sending a RESET
#define CBMCTRL_RESET       _CBMIO(CBMCTRL_BASE, 6)  // -                    -
//! IOCTL for getting the EOI state
#define CBMCTRL_GET_EOI     _CBMIO(CBMCTRL_BASE, 7)  // -                    CBMT_GET_EOI_OUT
//! IOCTL for resetting the EOI state
#define CBMCTRL_CLEAR_EOI   _CBMIO(CBMCTRL_BASE, 8)  // -                    -

//! IOCTL for reading the PP values
#define CBMCTRL_PP_READ     _CBMIO(CBMCTRL_BASE, 10) // -                    CBMT_PP_READ_OUT
//! IOCTL for setting the PP values
#define CBMCTRL_PP_WRITE    _CBMIO(CBMCTRL_BASE, 11) // CBMT_PP_WRITE_IN     -
//! IOCTL for polling an IEC line
#define CBMCTRL_IEC_POLL    _CBMIO(CBMCTRL_BASE, 12) // -                    CBMT_IEC_POLL_OUT
//! IOCTL for setting an IEC line
#define CBMCTRL_IEC_SET     _CBMIO(CBMCTRL_BASE, 13) // CBMT_IEC_SET_IN      -
//! IOCTL for releasing an IEC line
#define CBMCTRL_IEC_RELEASE _CBMIO(CBMCTRL_BASE, 14) // CBMT_IEC_RELEASE_IN  -
//! IOCTL for waiting for an IEC line
#define CBMCTRL_IEC_WAIT    _CBMIO(CBMCTRL_BASE, 15) // CBMT_IEC_WAIT_IN     CBMT_IEC_WAIT_OUT

//! IOCTL for performing und checking the installation; ONLY FOR USE OF INSTCBM!
#define CBMCTRL_I_INSTALL   _CBMIO(CBMCTRL_BASE, 16) // -                    CBMT_I_INSTALL_OUT (or an array of it)

#if DBG
//! IOCTL reading the debug buffer of the driver; ONLY FOR USE OF INSTCBM!
#define CBMCTRL_I_READDBG   _CBMIO(CBMCTRL_BASE, 17) // -                    char array which will be filled
#endif // #if DBG

//! IOCTL for setting and releasing IEC lines at once
#define CBMCTRL_IEC_SETRELEASE _CBMIO(CBMCTRL_BASE, 18) // CBMT_IEC_SETRELEASE_IN -

//! IOCTL for reading from the parallel port (for controlling parallel burst)
#define CBMCTRL_PARBURST_READ \
                            _CBMIO(CBMCTRL_BASE, 19) // CBMT_IEC_PARBURST_PREAD_IN -

//! IOCTL for writing to the parallel port (for controlling parallel burst)
#define CBMCTRL_PARBURST_WRITE \
                            _CBMIO(CBMCTRL_BASE, 20) // -                    CBMT_IEC_PARBURST_PWRITE_OUT

//! IOCTL for reading a complete track (for parallel burst)
#define CBMCTRL_PARBURST_READ_TRACK \
                            _CBMIO(CBMCTRL_BASE, 21) // -                    CBMT_PARBURST_READ_TRACK_OUT

//! IOCTL for writing a complete track (for parallel burst)
#define CBMCTRL_PARBURST_WRITE_TRACK \
                            _CBMIO(CBMCTRL_BASE, 22) // CBMT_PARBURST_WRITE_TRACK_IN -

//! IOCTL for locking the driver onto the parallel port
#define CBMCTRL_PARPORT_LOCK  \
                            _CBMIO(CBMCTRL_BASE, 23) // -                    -

//! IOCTL for locking the driver onto the parallel port
#define CBMCTRL_PARPORT_UNLOCK  \
                            _CBMIO(CBMCTRL_BASE, 24) // -                    -

//! IOCTL for updating settings of the driver
#define CBMCTRL_UPDATE \
                            _CBMIO(CBMCTRL_BASE, 25) // -                    -

//! IOCTL for testing the interrupt facilities of the hardware
#define CBMCTRL_TEST_IRQ \
                            _CBMIO(CBMCTRL_BASE, 26) // -                    -

//! IOCTL for debugging a cable: Read all the input lines in a hardware-specific way
#define CBMCTRL_IEC_DBG_READ  _CBMIO(CBMCTRL_BASE, 27) // CBMT_IEC_DBG_READ    -

//! IOCTL for debugging a cable: Write all the output lines in a hardware-specific way
#define CBMCTRL_IEC_DBG_WRITE _CBMIO(CBMCTRL_BASE, 28) // -                    CBMT_IEC_DBG_WRITE


//! IOCTL for reading a variable length track (for parallel burst)
#define CBMCTRL_PARBURST_READ_TRACK_VAR \
                            _CBMIO(CBMCTRL_BASE, 29) // -                    CBMT_PARBURST_READ_TRACK_OUT

/* these are the return codes of CBMCTRL_I_INSTALL: */

//! CBMCTRL_I_INSTALL: The driver could not be opened
#define CBM_I_DRIVER_INSTALL_0_FAILED         0xFFFFFFFF
//! CBMCTRL_I_INSTALL: The IOCTL failed
#define CBM_I_DRIVER_INSTALL_0_IOCTL_FAILED   0xFFFFFFFE
//! CBMCTRL_I_INSTALL: No interrupt available
#define CBM_I_DRIVER_INSTALL_0M_NO_INTERRUPT  0x80000000

#endif /* #ifndef CBM_IOCTL_H */
