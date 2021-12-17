/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2004-2006 Spiro Trikaliotis
 *  Copyright 1998      Wolfgang Moser (http://d81.de)
 *
 *  This file is heavily based on LPTDTC by Wolfgang Moser.
 */

/*! ************************************************************** 
** \file sys/nt4/PortAccessNt4.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Functions for communicating with the parallel port driver,
**        NT4 version
**
****************************************************************/

#include <initguid.h>
#include <wdm.h>
#include "cbm_driver.h"



/*! \brief search for unusual EPPs

  Set AdvancedEPPTests to 1 if you want to try searching for unusual EPPs
  But take notice that the functionality of this test is not guaranteed!
*/
#define AdvancedEPPTests 1

/*! the mode the parallel port is in */
enum lptMode
{
    lptN_A    = 0,  /*!< no parallel port */
    lptSPP    = 1,  /*!< SPP parallel port */
    lptPS2    = 2,  /*!< PS2 parallel port */
    lptEPP    = 3,  /*!< EPP parallel port */
    lptECP    = 4,  /*!< ECP parallel port */
    lptEPPc   = 5,  /*!< EPP with special control word to enable */
};

/*! the available ecp modes */
enum ecpMode
{
    ecpNOECR   = 0,  /*!< no ECP-Port found */

    ecpSTNDRD  = 1,  /*!< standard mode */
    ecpBYTE    = 2,  /*!< BYTE mode */
    ecpSTDFIFO = 3,  /*!< Standard FIFO mode */
    ecpECPFIFO = 4,  /*!< ECP FIFO mode */
    ecpEPP     = 5,  /*!< EPP mode */
    ecpRESVRD  = 6,  /*!< reserved mode */
    ecpFIFOTST = 7,  /*!< FIFO test mode */
    ecpCONFIG  = 8   /*!< config mode */
};

// set the Bidirectional-Flag to output (normal mode)
static void
BIDIoutp(PUCHAR port)
{
    WRITE_PORT_UCHAR(port+2,0xC4);
}

// set the Bidirectional-Flag to input (reverse mode)
static void
BIDIinp(PUCHAR port)
{
    WRITE_PORT_UCHAR(port+2,0xE4);
}

// clear the Timeout-Flag
static void
EPPclear(PUCHAR port)
{
    unsigned char val=READ_PORT_UCHAR(port+1);

                                          // Reset Timeout-Flag by reading
    WRITE_PORT_UCHAR(port+1,val|0x01);    // or by writing 0
    WRITE_PORT_UCHAR(port+1,val&0xFE);    // or by writing 1 to it
}

//----------------------------------------------------------------
// Port Detection Routines
//----------------------------------------------------------------

// ECP port detection
static enum lptMode
ECPdetect(PUCHAR port)
{
    enum lptMode ret = lptN_A;

    unsigned char ECR = READ_PORT_UCHAR(port+0x402)&~0x07;
    do
    {
        WRITE_PORT_UCHAR(port+0x402,0x34);
        WRITE_PORT_UCHAR(port+2,0xC6);
        if(READ_PORT_UCHAR(port+0x402)!=0x35){
        ECR=0xC4;                            // There's no ECR,
        break;
    }

    WRITE_PORT_UCHAR(port+0x402,0x35);
    WRITE_PORT_UCHAR(port+0x402,0xd4);
    READ_PORT_UCHAR(port+0x400);
    WRITE_PORT_UCHAR(port+0x400,0xAA);
    if(READ_PORT_UCHAR(port+0x400)!=0xAA)
        break;

    WRITE_PORT_UCHAR(port+0x400,0x55);
    if(READ_PORT_UCHAR(port+0x400)!=0x55)
        break;

    ret=lptECP;

    } while(0);

    WRITE_PORT_UCHAR(port+0x402,0x35);
    WRITE_PORT_UCHAR(port+0x402,ECR);
    return ret;
}

// EPP port detection without Control port initialisation
static enum lptMode
EPPdWOC(PUCHAR port)
{
    do
    {
        EPPclear(port);
        WRITE_PORT_UCHAR(port+3,0xAA);
        EPPclear(port);
        if(READ_PORT_UCHAR(port+3)!=0xAA)
            break;

        EPPclear(port);
        WRITE_PORT_UCHAR(port+3,0x55);
        EPPclear(port);

        if(READ_PORT_UCHAR(port+3)!=0x55)
            break;

        return lptEPP;

    } while(0);

    EPPclear(port);
    WRITE_PORT_UCHAR(port+3,0x00);
    READ_PORT_UCHAR(port+3);

    if(!(READ_PORT_UCHAR(port+1)&0x01))
        return lptN_A;

    EPPclear(port);
    if(READ_PORT_UCHAR(port+1)&0x01)
        return lptN_A;

    return lptEPP;
}

// set the Bidirectional-Flag to input (reverse mode) and block
// the DataStrobe, AddressStrobe and Write line manually, so
// that the EPP can't send any automatic handshake signal
static enum lptMode
EPPdetect(PUCHAR port)
{
    WRITE_PORT_UCHAR(port+2, (UCHAR) 0xEF);
    return EPPdWOC(port);
}

// Parallel Printer Port detection (SPP or PS/2)
static enum lptMode
PPPdetect(PUCHAR port)
{
    BIDIoutp(port);

    WRITE_PORT_UCHAR(port,0xAA);
    if(READ_PORT_UCHAR(port)!=0xAA)
        return lptN_A;

    WRITE_PORT_UCHAR(port,0x55);
    if(READ_PORT_UCHAR(port)!=0x55)
        return lptN_A;

    BIDIinp(port);

    WRITE_PORT_UCHAR(port,0xAA);
    if(READ_PORT_UCHAR(port)!=0xAA)
        return lptPS2;

    WRITE_PORT_UCHAR(port,0x55);
    if(READ_PORT_UCHAR(port)!=0x55)
        return lptPS2;

    return lptSPP;
}

// perform a parallel printer port reset
static void
ResetLPT(PUCHAR port)
{
    int i;

    // since a port read/write command is delayed by ISA bus waitstates to
    // 1,6 æs, we can use it for a simple system independent delay routine.

    // But it would be much better to program one of the system
    // timers to delay 16 micro seconds
    for(i=10;i>0;i--)
        WRITE_PORT_UCHAR(port+2,0xC0);

    BIDIoutp(port);
}

//----------------------------------------------------------------
// Port Mode Resolving Routines
//----------------------------------------------------------------

#if AdvancedEPPTests
/*! \internal \brief Test for advanced EPP (with different control words)

 \param port
    (Base) port number of parallel port to test for advanced EPP

 \return
    lptEPPc if advanced EPP port has been found; else lptN_A.
*/
static enum lptMode AdvEPP(PUCHAR port){
    unsigned char EPPctrl=0;

    // check all control words to free up the EPP
    do
    {
        EPPctrl |= 0x04;    // don't do a reset
        // write the special Control word, for freeing up the EPP
        WRITE_PORT_UCHAR(port+2,EPPctrl);

        if (EPPdWOC(port)!=lptN_A)
            return lptEPPc;

        EPPctrl++;
    } while(EPPctrl);

    return lptN_A;
}

// Resolving the special Control-Word to enable an EPP
static char *
EPPcontrol(PUCHAR port){
    int ret;
    static char ctrlWord[8];
    unsigned int  done0[8], done1[8];
    unsigned char i,EPPctrl, OldCtrl,mask;

    for (i=0;i<8;i++)
        done0[i]=done1[i]=0;

    OldCtrl = READ_PORT_UCHAR(port+2)&0x1f;
    EPPctrl = 0;  // check all control words to free up the EPP

    do
    {
        EPPctrl|=0x04;    // don't do a reset
        // write the special Control word, for freeing up the EPP
        WRITE_PORT_UCHAR(port+2,EPPctrl);
        if(EPPdWOC(port)!=lptN_A)
        {
            // EPP is enabled with this control word
            // return EPPctrl;
            for(i=0,mask=0x80;i<8;i++,mask>>=1)
            {
                if(EPPctrl&mask)
                    done1[i]++;
                else 
                    done0[i]++;
            }
        }
        EPPctrl++;
    } while(EPPctrl);

    WRITE_PORT_UCHAR(port+2,OldCtrl);

    for (i=0;i<8;i++)
    {
        if(done0[i]==done1[i])
        {
            if(!done0[i])      ctrlWord[i]='!';    // Control-Word could not found
            else                   ctrlWord[i]='X';    // This Bit cares nobody
        }
        else if(!done0[i]) ctrlWord[i]='1'; // This Bit must be 1
        else if(!done1[i]) ctrlWord[i]='0';    // This Bit must be 0
        else               ctrlWord[i]='?';    // This Bit depends on other Bits
    }
    return ctrlWord;
}
#else
static char *
EPPcontrol(PUCHAR)
{
    return "XX0X0100";
}
#endif

static enum lptMode
LPTmode(PUCHAR port)
{
    enum lptMode ret=lptN_A;

    // check for valid portaddresses (LPT 1-6)
    // valid port addresses only at 0x200, 0x204, 0x208, ..., 0x3fc

    if((((ULONG_PTR)port) & ~0x1fc)!=0x200)
        return lptN_A;

    //    if(!(port&0x07)){
    // test for ECP/EPP only at 0/8-bases

    ResetLPT(port);
    //    ECP test doesn't touch any data registers, so no reset is needed

    do
    {
        // tests for an ECP
        if((ret = ECPdetect(port)) != lptN_A)
            break;

        // perform a reset to prevent printers from printing unusable stuff
        ResetLPT(port);

        // tests for an EPP
        if ((ret = EPPdetect(port)) != lptN_A)
            break;

#if AdvancedEPPTests
        // tests for an EPP with different control words
        if ((ret = AdvEPP(port)) != lptN_A)
            break;
#endif
        // tests for a SPP or PS/2
        if ((ret = PPPdetect(port)) != lptN_A)
            break;

    } while(0);
    BIDIoutp(port);
    return ret;
}

static enum ecpMode
ECPmode(PUCHAR port)
{
    if (LPTmode(port) != lptECP)
        return ecpNOECR;

    return (enum ecpMode)(ecpSTNDRD + (READ_PORT_UCHAR(port+0x402)>>5));
}

static enum ecpMode
SetECPmode(PUCHAR port, enum ecpMode mode)
{
    unsigned char oldvalue;

    if (LPTmode(port) != lptECP)
        return ecpNOECR;

    oldvalue = READ_PORT_UCHAR(port+0x402);

    WRITE_PORT_UCHAR(port+0x402, (oldvalue & 0x3f) | ((mode - ecpSTNDRD) << 5));

    return (enum ecpMode)(ecpSTNDRD + (oldvalue>>5));
}

//----------------------------------------------------------------
// Some helper functions
//----------------------------------------------------------------

static int
preprprt(PUCHAR port)
{
    enum lptMode mode;
    const char *modes[6]={"N/A","SPP","PS/2","EPP","ECP","EPPc"};

    FUNC_ENTER();

    mode=LPTmode(port);
    DBG_PRINT((DBG_PREFIX "at 0x%03X, %s", port, modes[mode]));

    FUNC_LEAVE_INT(mode);
}

static int
prport(PUCHAR port)
{
    const char *ecpM[9]={
        "no ECR found",
        "Standard Mode",
        "Byte Mode",
        "Parallel Port FIFO Mode",
        "ECP FIFO Mode",
        "EPP Mode",
        "Reserved",
        "FIFO Test Mode",
        "Configuration Mode"
    };

    int ret;

    FUNC_ENTER();

    ret=1;

    switch (preprprt(port))
    {
        case lptN_A:
            ret=0;
            break;
        case lptEPPc:
            DBG_PRINT((DBG_PREFIX ",  EPP-Enable-Timeout-Bit-Detection-Control-Word:"
                " %s", EPPcontrol(port)));
            break;

        case lptECP:
            DBG_PRINT((DBG_PREFIX ",  ECP-Mode: %s", ecpM[ECPmode(port)]));
    }
    DBG_PRINT((DBG_PREFIX ""));

    FUNC_LEAVE_INT(ret);
}


/*! \brief Set the operational mode of the parallel port, NT4 version

 This function sets the operational mode of the parallel port.

 \param Pdx
   Pointer to a device extension which contains the DEVICE_OBJECT 
   of the parallel port driver.

 This function has to be balanced with a corresponding ParPortUnsetModeNt4()

 This function must be run at IRQL == PASSIVE_LEVEL.
*/
NTSTATUS
ParPortSetModeNt4(PDEVICE_EXTENSION Pdx)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    enum lptMode lptmode = Pdx->HandleEcpEppMyself & 0xFF;
    enum ecpMode oldecpmode = ecpNOECR;

    FUNC_ENTER();

    if (lptmode == lptN_A)
    {
        DBG_PRINT((DBG_PREFIX "Trying to find out mode of 0x%04x", Pdx->ParPortPortAddress));
        lptmode = LPTmode(Pdx->ParPortPortAddress);
        prport(Pdx->ParPortPortAddress);
    }

    if (lptmode == lptECP)
    {
        oldecpmode = ECPmode(Pdx->ParPortPortAddress);

        DBG_PRINT((DBG_PREFIX "Setting ECP mode to BYTE mode"));
        SetECPmode(Pdx->ParPortPortAddress, ecpBYTE);

        DBG_PRINT((DBG_PREFIX "Checking if mode has changed:"));
        prport(Pdx->ParPortPortAddress);
    }

    if (lptmode > lptSPP)
    {
        DBG_PRINT((DBG_PREFIX "Writing the value 0xE4 to 0x%04x", Pdx->ParPortPortAddress+2));
        WRITE_PORT_UCHAR(Pdx->ParPortPortAddress+2,0xE4);
    }

    Pdx->HandleEcpEppMyself = (lptmode & 0xFF) | ((oldecpmode & 0xFF) << 8);

    FUNC_LEAVE_NTSTATUS(ntStatus);
}

/*! \brief Unset the operational mode of the parallel port, NT4 version

 This function unsets the operational mode of the parallel port.

 \param Pdx
   Pointer to a device extension which contains the DEVICE_OBJECT 
   of the parallel port driver.

 This function mustn't be called without a prior call to
 ParPortSetModeNt4()

 This function must be run at IRQL == PASSIVE_LEVEL.
*/
NTSTATUS
ParPortUnsetModeNt4(PDEVICE_EXTENSION Pdx)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    enum lptMode lptmode = Pdx->HandleEcpEppMyself & 0xFF;
    enum ecpMode oldecpmode = (Pdx->HandleEcpEppMyself & 0xFF00) >> 8;

    FUNC_ENTER();

    if (lptmode == lptECP)
    {
        DBG_PRINT((DBG_PREFIX "Setting ECP mode back to standard mode"));
        SetECPmode(Pdx->ParPortPortAddress, oldecpmode);

        DBG_PRINT((DBG_PREFIX "Checking if mode has changed:"));
        prport(Pdx->ParPortPortAddress);
    }

    FUNC_LEAVE_NTSTATUS(ntStatus);
}

extern VOID
cbmiec_udelay(IN ULONG howlong); // howlong in ms!

/*! \brief Set the operational mode of the parallel port

 This function sets the operational mode of the parallel port.

 \param Pdx
   Pointer to a device extension which contains the DEVICE_OBJECT 
   of the parallel port driver.

 This function has to be balanced with a corresponding ParPortUnsetMode()

 This function must be run at IRQL == PASSIVE_LEVEL.
*/
NTSTATUS
ParPortSetMode(PDEVICE_EXTENSION Pdx)
{
    NTSTATUS ntStatus = STATUS_LOGON_FAILURE; // this is just a dummy failure value to allow falling into NT4 processing

    FUNC_ENTER();

    if (!Pdx->HandleEcpEppMyself)
    {
        // First, try to set the mode via WDM functions.
        // Remember: Even if we install the NT4 driver, we might be running on 2000 or up.
        ntStatus = ParPortSetModeWdm(Pdx);
    }

    if (Pdx->HandleEcpEppMyself || ((ntStatus == STATUS_INVALID_PARAMETER) && Pdx->IsNT4))
    {
        ntStatus = ParPortSetModeNt4(Pdx);
        cbmiec_udelay(1000 * 10);
    }

    FUNC_LEAVE_NTSTATUS(ntStatus);
}

/*! \brief Unset the operational mode of the parallel port

 This function unsets the operational mode of the parallel port.

 \param Pdx
   Pointer to a device extension which contains the DEVICE_OBJECT 
   of the parallel port driver.

 This function mustn't be called without a prior call to
 ParPortSetMode()

 This function must be run at IRQL == PASSIVE_LEVEL.
*/
NTSTATUS
ParPortUnsetMode(PDEVICE_EXTENSION Pdx)
{
    NTSTATUS ntStatus;

    FUNC_ENTER();

    if (Pdx->HandleEcpEppMyself == 0)
    {
        ntStatus = ParPortUnsetModeWdm(Pdx);
    }
    else
    {
        ntStatus = ParPortUnsetModeNt4(Pdx);
    }

    FUNC_LEAVE_NTSTATUS(ntStatus);
}
