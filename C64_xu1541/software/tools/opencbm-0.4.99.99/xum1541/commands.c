/*
 * Handle USB bulk and control transactions from the host
 * Copyright (c) 2009-2010 Nate Lawson <nate@root.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#include "xum1541.h"

/*
 * Basic inline IO functions where each byte is processed as it is
 * received. This decreases latency, especially for protocols like
 * nibbler.
 */
typedef uint8_t (*ReadFn_t)(void);
typedef void (*WriteFn_t)(uint8_t data);
typedef void (*Read2Fn_t)(uint8_t *data);
typedef void (*Write2Fn_t)(uint8_t *data);

// Track a transfer between usbInitIo()/usbIoDone().
static uint16_t usbDataLen;
static uint8_t usbDataDir = XUM_DATA_DIR_NONE;

// Are we in the middle of a command sequence (XUM1541_INIT .. SHUTDOWN)?
#define XUM1541_CMD_IN_PROGRESS 0x80
static uint8_t cmdSeqInProgress;

// Current device state for the XUM1541_INIT response
static uint8_t currState;

// Nibtools command state. See nib_parburst_read/write_checked()
static bool suppressNibCmd;
static uint8_t savedNibWrites[4], *savedNibWritePtr;

// Protocol handlers to use, set in cbm_init().
struct ProtocolFunctions *cmds;

static int nib_check_write(uint8_t data);

// Allow setting tracking var usbDataLen from outside.
void Set_usbDataLen(uint16_t Len) { usbDataLen = Len; }

/*
 * Probe for CBM 153x tape device first. If found enter tape mode and
 * return. If not found or unsupported, continue device detection.
 * Try to find an IEEE device next. If not found or unsupported, fall
 * back to IEC.
 */
struct ProtocolFunctions *
cbm_init(void)
{
    struct ProtocolFunctions *protoFn = NULL;

    // Nothing going on with the device yet.
    currState = 0;

#ifdef TAPE_SUPPORT
    if (Probe4TapeDevice() == Tape_Status_OK_Tape_Device_Present)
    {
        Enter_Tape_Mode(&protoFn);
        currState |= XUM1541_TAPE_PRESENT;
        return protoFn;
    }
#endif

#ifdef IEEE_SUPPORT
    protoFn = ieee_init();
    if (protoFn != NULL) {
        currState |= XUM1541_IEEE488_PRESENT;
        return protoFn;
    }
#endif

    // Always use IEC as last resort.
    board_init_iec();
    protoFn = iec_init();
    return protoFn;
}

void
usbInitIo(uint16_t len, uint8_t dir)
{
#ifdef DEBUG
    if (usbDataDir != XUM_DATA_DIR_NONE)
        DEBUGF(DBG_ERROR, "ERR: usbInitIo left in bad state %d\n", usbDataDir);
#endif

    // Select the proper endpoint for this direction
    if (dir == ENDPOINT_DIR_IN) {
        Endpoint_SelectEndpoint(XUM_BULK_IN_ENDPOINT);
    } else if (dir == ENDPOINT_DIR_OUT) {
        Endpoint_SelectEndpoint(XUM_BULK_OUT_ENDPOINT);
    } else {
        DEBUGF(DBG_ERROR, "ERR: usbInitIo bad dir %d\n");
        return;
    }

    usbDataLen = len;
    usbDataDir = dir;

    /*
     * Wait until endpoint is ready before continuing. It is critical
     * that we do this so that the transfer routines have somewhat
     * minimal latency when accessing the endpoint buffers. Otherwise,
     * timing could be violated.
     */
    while (!Endpoint_IsReadWriteAllowed())
        ;
}

void
usbIoDone(void)
{
    // Finalize any outstanding transactions
    if (usbDataDir == ENDPOINT_DIR_IN) {
        /*
         * If the transfer left an incomplete endpoint (mod endpoint size)
         * or possibly never transferred any data (error or timeout case),
         * flush the buffer back to the host.
         */
        if (Endpoint_BytesInEndpoint() != 0 || usbDataLen != 0) {
            Endpoint_ClearIN();
        }
    } else if (usbDataDir == ENDPOINT_DIR_OUT) {
        /*
         * If we didn't consume all data from the host, then discard it now.
         * Just clearing the endpoint (below) works fine if the remaining
         * data is less than the endpoint size, but would leave data in
         * the buffer if there was more.
         */
        if (usbDataLen != 0)
            Endpoint_Discard_Stream(usbDataLen, AbortOnReset);

        /*
         * Request another buffer from the host. If it has one, it will
         * be ready when we start the next transfer.
         */
        if (!Endpoint_IsReadWriteAllowed())
            Endpoint_ClearOUT();
    } else {
        DEBUGF(DBG_ERROR, "done: bad io dir %d\n", usbDataDir);
    }
    usbDataDir = XUM_DATA_DIR_NONE;
    usbDataLen = 0;
}

int8_t
usbSendByte(uint8_t data)
{

#ifdef DEBUG
    if (usbDataDir != ENDPOINT_DIR_IN) {
        DEBUGF(DBG_ERROR, "ERR: usbSendByte when dir was %d\n", usbDataDir);
        return -1;
    }
#endif

    // Write data back to the host buffer for USB transfer
    Endpoint_Write_Byte(data);
    usbDataLen--;

    // If the endpoint is now full, flush the block to the host
    if (!Endpoint_IsReadWriteAllowed()) {
        Endpoint_ClearIN();
        while (!Endpoint_IsReadWriteAllowed() && !doDeviceReset)
            ;
    }

    // Check if the current command is being aborted by the host
    if (doDeviceReset) {
        DEBUGF(DBG_ERROR, "sndrst\n");
        return -1;
    }

    return 0;
}

int8_t
usbRecvByte(uint8_t *data)
{

#ifdef DEBUG
    if (usbDataDir != ENDPOINT_DIR_OUT) {
        DEBUGF(DBG_ERROR, "ERR: usbRecvByte when dir was %d\n", usbDataDir);
        return -1;
    }
#endif

    /*
     * Check if the endpoint is currently empty.
     * If so, clear the endpoint bank to get more data from host and
     * wait until the host has sent data.
     */
    if (!Endpoint_IsReadWriteAllowed()) {
        Endpoint_ClearOUT();
        while (!Endpoint_IsReadWriteAllowed() && !doDeviceReset)
            ;
    }

    // Check if the current command is being aborted by the host
    if (doDeviceReset) {
        DEBUGF(DBG_ERROR, "rcvrst\n");
        return -1;
    }

    // Read data from the host buffer, received via USB
    *data = Endpoint_Read_Byte();
    usbDataLen--;

    return 0;
}

static uint8_t
ioReadLoop(ReadFn_t readFn, uint16_t len)
{
    uint8_t data;

    usbInitIo(len, ENDPOINT_DIR_IN);
    while (len-- != 0) {
        data = readFn();
        if (usbSendByte(data) != 0)
            break;
    }
    usbIoDone();
    return 0;
}

static uint8_t
ioWriteLoop(WriteFn_t writeFn, uint16_t len)
{
    uint8_t data;

    usbInitIo(len, ENDPOINT_DIR_OUT);
    while (len-- != 0) {
        if (usbRecvByte(&data) != 0)
            break;
        writeFn(data);
    }
    usbIoDone();
    return 0;
}

static uint8_t
ioRead2Loop(Read2Fn_t readFn, uint16_t len)
{
    uint8_t data[2];

    usbInitIo(len, ENDPOINT_DIR_IN);
    while (len != 0) {
        readFn(data);
        if (usbSendByte(data[0]) != 0 || usbSendByte(data[1]) != 0)
            break;
        len -= 2;
    }
    usbIoDone();
    return 0;
}

static uint8_t
ioWrite2Loop(Write2Fn_t writeFn, uint16_t len)
{
    uint8_t data[2];

    usbInitIo(len, ENDPOINT_DIR_OUT);
    while (len != 0) {
        if (usbRecvByte(&data[0]) != 0 || usbRecvByte(&data[1]) != 0)
            break;
        writeFn(data);
        len -= 2;
    }
    usbIoDone();
    return 0;
}

static uint8_t
ioReadNibLoop(uint16_t len, bool earlyExit)
{
    uint16_t i;
    uint8_t data;

    // Probably an error, but handle it anyway.
    if (len == 0)
        return 0;

    suppressNibCmd = false;
    usbInitIo(len, ENDPOINT_DIR_IN);
    iec_release(IO_DATA);

    /*
     * Wait until the host has gotten to generating an IN transaction
     * before doing the handshake that leads to data beginning to stream
     * out. Not sure the exact value for this, but 5 ms appears to fail.
     */
    DELAY_MS(10);

    // We're ready to go, kick off the actual data transfer
    nib_parburst_read();

    for (i = 0; i < len; i++) {
        // Read a byte from the parport
        if (nib_read_handshaked(&data, i & 1) < 0) {
            DEBUGF(DBG_ERROR, "nbrdth1 to %d\n", i);
            return -1;
        }

        // Send the byte via USB
        if (usbSendByte(data) != 0)
            break;

        // If requested, terminate early on seeing a special marker.
        // This is used by burst_read_track_var() to scan density.
        if (earlyExit && data == 0x55)
            break;
    }
    usbIoDone();

    // All bytes read ok so read the final dummy byte
    nib_parburst_read();
    return 0;
}

static uint8_t
ioWriteNibLoop(uint16_t len)
{
    uint16_t i;
    uint8_t data, *ptr;

    // Probably an error, but handle it anyway.
    if (len == 0)
        return 0;

    suppressNibCmd = false;
    usbInitIo(len, ENDPOINT_DIR_OUT);
    iec_release(IO_DATA);

    /*
     * We're ready to go, kick off the actual data transfer by writing
     * all saved writes. We keep a queue because the 1541 and 1571 drive
     * code may send different numbers of bytes here.
     */
    for (ptr = savedNibWrites; ptr != savedNibWritePtr; ptr++)
        nib_parburst_write(*ptr);
    savedNibWritePtr = savedNibWrites;

    for (i = 0; i < len; i++) {
        // Get the byte via USB
        if (usbRecvByte(&data) != 0)
            break;

        // Write a byte to the parport
        if (nib_write_handshaked(data, i & 1) < 0) {
            DEBUGF(DBG_ERROR, "nbwrh1 to\n");
            return -1;
        }
    }

    /*
     * All bytes written ok, so write a final zero byte and read back the
     * dummy result.
     */
    nib_write_handshaked(0, i & 1);
    nib_parburst_read();

    usbIoDone();
    return 0;
}

#ifdef SRQ_NIB_SUPPORT
static uint8_t
ioReadNibSrqLoop(uint16_t len)
{
    uint16_t i;
    uint8_t data;

    // Probably an error, but handle it anyway.
    if (len == 0)
        return 0;

    suppressNibCmd = false;
    usbInitIo(len, ENDPOINT_DIR_IN);
    iec_release(IO_SRQ | IO_CLK | IO_DATA | IO_ATN);

    /*
     * Wait until the host has gotten to generating an IN transaction
     * before doing the handshake that leads to data beginning to stream
     * out. This value is derived from the parallel nib routine.
     */
    DELAY_MS(10);

    // We're ready to go, kick off the actual data transfer
    nib_srqburst_read();

    // Start signal for drive code
    iec_set(IO_CLK);

    for (i = 0; i < len; i++) {
        // Read a byte via srq
        data = iec_srq_read();

        // Stop signal for drive code is to release CLK on last byte.
        if (i == (len - 2))
            iec_release(IO_CLK);

        // Send the byte to the host via USB
        if (usbSendByte(data) != 0)
            break;
    }
    usbIoDone();

    // All bytes read ok so read the final dummy byte
    nib_srqburst_read();

    return 0;
}

static uint8_t
ioWriteNibSrqLoop(uint16_t len)
{
    uint16_t i;
    uint8_t data, *ptr;

    // nibtools drive code requires at least one data byte.
    if (len == 0)
        return 0;

    suppressNibCmd = false;
    usbInitIo(len, ENDPOINT_DIR_OUT);
    iec_release(IO_SRQ | IO_CLK | IO_DATA | IO_ATN);

    /*
     * We're ready to go, kick off the actual data transfer by writing
     * all saved writes. We keep a queue because the 1541 and 1571 drive
     * code may send different numbers of bytes here.
     */
    for (ptr = savedNibWrites; ptr != savedNibWritePtr; ptr++)
        nib_srqburst_write(*ptr);
    savedNibWritePtr = savedNibWrites;

    for (i = 0; i < len; i++) {
        // Get data byte from USB.
        if (usbRecvByte(&data) != 0)
            break;

        // Write data byte via SRQ, break if timeout error.
        if (nib_srq_write_handshaked(data, i & 1) != 0) {
            DEBUGF(DBG_ERROR, "nbwrh1 to\n");
            return -1;
        }
    }

    // Read back the dummy result.
    nib_srqburst_read();

    usbIoDone();
    return 0;
}

// Check with the state machine before actually doing the write
static void
nib_srqburst_write_checked(uint8_t data)
{
    if (nib_check_write(data))
        nib_srqburst_write(data);
}

/*
 * Delay the handshaked read until read/write track, if that's the
 * next function to run. nib_parburst_write_checked() sets this flag.
 */
static uint8_t
nib_srqburst_read_checked(void)
{
    if (!suppressNibCmd)
        return nib_srqburst_read();
    else
        return 0x88;
}
#endif // SRQ_NIB_SUPPORT

// Check with the state machine before actually doing the write
static void
nib_parburst_write_checked(uint8_t data)
{
    if (nib_check_write(data))
        nib_parburst_write(data);
}

/*
 * Delay the handshaked read until read/write track, if that's the
 * next function to run. nib_parburst_write_checked() sets this flag.
 */
static uint8_t
nib_parburst_read_checked(void)
{
    if (!suppressNibCmd)
        return nib_parburst_read();
    else
        return 0x88;
}

/*
 * Set a flag to suppress the next nib_parburst_read(). We do this if
 * the current mnib command is to read or write a track-at-once.
 *
 * Since timing is critical for the toggled read/write, we need to be
 * ready to poll in a tight loop before starting it. However, nibtools
 * expects to write the 00,55,aa,ff command, then read a handshaked byte
 * to ack that it is ready, then finally do the polled loop itself.
 *
 * With USB, the transition from the handshake to the actual polled loop
 * can be tens of milliseconds. So we suppress the handshaked read that
 * is initiated by nibtools, then do it ourselves when we really are ready
 * (in ioReadNibLoop(), above). This ensures that there is minimal delay
 * between the handshake and the polling loop.
 *
 * For write track, this is similar except that final ack writes a single
 * byte that indicates how to align tracks, if at all. We cache that for
 * ioWriteNibLoop().
 */
static int
nib_check_write(uint8_t data)
{
    static uint8_t mnibCmd[] = { 0x00, 0x55, 0xaa, 0xff };
    static uint8_t cmdIdx;

    /*
     * If cmd is a write track, save up to 4 bytes of data that will be
     * sent later but suppress the handshaked write for now.
     */
    if (suppressNibCmd) {
        *savedNibWritePtr++ = data;
        return false;
    }

    // State machine to match 00,55,aa,ff,XX where XX is read/write track.
    if (cmdIdx == sizeof(mnibCmd)) {
        if (data == 0x03 || data == 0x04 || data == 0x05 || data == 0x0b ||
            data == 0x13 || data == 0x14 || data == 0x16) {
            suppressNibCmd = true;
        }
        cmdIdx = 0;
    } else if (mnibCmd[cmdIdx] == data) {
        cmdIdx++;
    } else {
        cmdIdx = 0;
        if (mnibCmd[0] == data)
            cmdIdx++;
    }

    return true;
}

/*
 * Delay a little (required), shutdown USB, disable watchdog and interrupts,
 * and jump to the bootloader.
 */
static void
enterBootLoader(void)
{
    // Control request was handled, so ack it
    Endpoint_ClearSETUP();
    while (!Endpoint_IsINReady())
        ;
    Endpoint_ClearIN();

    // Now shut things down and jump to the bootloader.
    DELAY_MS(100);
    wdt_disable();
    USB_ShutDown();
    cli();
    cpu_bootloader_start();
}

/*
 * Process the given USB control command, storing the result in replyBuf
 * and returning the number of output bytes. Returns -1 if command is
 * invalid. All control processing has to happen until completion (no
 * delayed execution) and it may not take additional input from the host
 * (data direction set as host to device).
 *
 * The replyBuf is 8 bytes long.
 */
int8_t
usbHandleControl(uint8_t cmd, uint8_t *replyBuf)
{
    DEBUGF(DBG_INFO, "cmd %d (%d)\n", cmd, cmd - XUM1541_IOCTL);

    switch (cmd) {
    case XUM1541_ENTER_BOOTLOADER:
        enterBootLoader();
        return 0;
    case XUM1541_ECHO:
        replyBuf[0] = cmd;
        return 1;
    case XUM1541_INIT:
        savedNibWritePtr = savedNibWrites;
        board_set_status(STATUS_ACTIVE);

        // First time: init IO pins and probe for IEC or IEEE devices
        if (cmds == NULL)
            cmds = cbm_init();
        // If we still don't have a bus, something is really wrong.
        if (cmds == NULL)
            return -1;

        replyBuf[0] = XUM1541_VERSION;
        replyBuf[1] = XUM1541_CAPABILITIES;
        replyBuf[2] = currState;

        /*
         * Our previous transaction was interrupted in the middle, say by
         * the user pressing ^C. Reset the IEC bus and then enter the
         * stalled state. The host will clear the stall and continue
         * their new transaction.
         */
        if (cmdSeqInProgress) {
            replyBuf[2] |= XUM1541_DOING_RESET;
            cmdSeqInProgress = XUM1541_DOING_RESET;
            cmds->cbm_reset(false);
            SetAbortState();
        }
        cmdSeqInProgress |= XUM1541_CMD_IN_PROGRESS;

        /*
         * We wait in main() until at least one device is present on the
         * IEC bus. Notify the user if they requested a command before
         * that has been detected.
         */
        if (!device_running)
            replyBuf[2] |= XUM1541_NO_DEVICE;
        return 8;
    case XUM1541_SHUTDOWN:
        cmdSeqInProgress = 0;
        board_set_status(STATUS_READY);
        return 0;
    case XUM1541_RESET:
        // Only do reset if we didn't just reset in INIT (above).
        if ((cmdSeqInProgress & XUM1541_DOING_RESET) == 0)
            cmds->cbm_reset(false);
        return 0;
#ifdef TAPE_SUPPORT
    case XUM1541_TAP_BREAK:
        cmds->cbm_reset(false);
        return 0;
#endif // TAPE_SUPPORT
    default:
        DEBUGF(DBG_ERROR, "ERR: control cmd %d not impl\n", cmd);
        return -1;
    }
}

// Store the 16-bit response to a bulk command in a status buffer.
#define XUM_SET_STATUS_VAL(buf, v)  *(uint16_t *)((buf) + 1) = (v)

int8_t
usbHandleBulk(uint8_t *request, uint8_t *status)
{
    uint8_t cmd, proto;
    int8_t ret;
    uint16_t len;
    bool nibEarlyExit;

    // Clear off "just did reset" flag each time a different cmd is run.
    cmdSeqInProgress &= ~XUM1541_DOING_RESET;

    // Default is to return no data
    ret = XUM1541_IO_READY;
    cmd = request[0];
    len = *(uint16_t *)&request[2];
    board_set_status(STATUS_ACTIVE);
    switch (cmd) {
    case XUM1541_READ:
        // Disallow any other protocols if in IEEE mode.
        if ((currState & XUM1541_IEEE488_PRESENT) == 0)
            proto = XUM_RW_PROTO(request[1]);
        else
            proto = XUM1541_CBM;
        DEBUGF(DBG_INFO, "rd:%d %d\n", proto, len);
        // loop to read all the bytes now, sending back each as we get it
        switch (proto) {
        case XUM1541_CBM:
            cmds->cbm_raw_read(len);
            ret = 0;
            break;
        case XUM1541_S1:
            ioReadLoop(s1_read_byte, len);
            ret = 0;
            break;
        case XUM1541_S2:
            ioReadLoop(s2_read_byte, len);
            ret = 0;
            break;
        case XUM1541_PP:
            ioRead2Loop(pp_read_2_bytes, len);
            ret = 0;
            break;
        case XUM1541_P2:
            ioReadLoop(p2_read_byte, len);
            ret = 0;
            break;
        case XUM1541_NIB:
            nibEarlyExit = (len & XUM1541_NIB_READ_VAR);
            len &= ~XUM1541_NIB_READ_VAR;
            ioReadNibLoop(len, nibEarlyExit);
            ret = 0;
            break;
        case XUM1541_NIB_COMMAND:
            ioReadLoop(nib_parburst_read_checked, len);
            ret = 0;
            break;
#ifdef SRQ_NIB_SUPPORT
        case XUM1541_NIB_SRQ:
            ioReadNibSrqLoop(len);
            ret = 0;
            break;
        case XUM1541_NIB_SRQ_COMMAND:
            ioReadLoop(nib_srqburst_read_checked, len);
            ret = 0;
            break;
#endif // SRQ_NIB_SUPPORT
#ifdef TAPE_SUPPORT
        case XUM1541_TAP:
            XUM_SET_STATUS_VAL(status, Tape_Capture());
            break;
        case XUM1541_TAP_CONFIG:
            XUM_SET_STATUS_VAL(status, Tape_DownloadConfig());
            break;
#endif // TAPE_SUPPORT
        default:
            DEBUGF(DBG_ERROR, "badproto %d\n", proto);
            ret = -1;
        }
        break;
    case XUM1541_WRITE:
        // Disallow any other protocols if in IEEE mode.
        if ((currState & XUM1541_IEEE488_PRESENT) == 0)
            proto = XUM_RW_PROTO(request[1]);
        else
            proto = XUM1541_CBM;
        DEBUGF(DBG_INFO, "wr:%d %d\n", proto, len);
        // loop to fetch each byte and write it as we get it
        switch (proto) {
        case XUM1541_CBM:
            len = cmds->cbm_raw_write(len, XUM_RW_FLAGS(request[1]));
            XUM_SET_STATUS_VAL(status, len);
            break;
        case XUM1541_S1:
            ioWriteLoop(s1_write_byte, len);
            ret = 0;
            break;
        case XUM1541_S2:
            ioWriteLoop(s2_write_byte, len);
            ret = 0;
            break;
        case XUM1541_PP:
            ioWrite2Loop(pp_write_2_bytes, len);
            ret = 0;
            break;
        case XUM1541_P2:
            ioWriteLoop(p2_write_byte, len);
            ret = 0;
            break;
        case XUM1541_NIB:
            ioWriteNibLoop(len);
            ret = 0;
            break;
        case XUM1541_NIB_COMMAND:
            ioWriteLoop(nib_parburst_write_checked, len);
            ret = 0;
            break;
#ifdef SRQ_NIB_SUPPORT
        case XUM1541_NIB_SRQ:
            ioWriteNibSrqLoop(len);
            ret = 0;
            break;
        case XUM1541_NIB_SRQ_COMMAND:
            ioWriteLoop(nib_srqburst_write_checked, len);
            ret = 0;
            break;
#endif // SRQ_NIB_SUPPORT
#ifdef TAPE_SUPPORT
        case XUM1541_TAP:
            XUM_SET_STATUS_VAL(status, Tape_Write());
            break;
        case XUM1541_TAP_CONFIG:
            XUM_SET_STATUS_VAL(status, Tape_UploadConfig());
            break;
#endif // TAPE_SUPPORT
        default:
            DEBUGF(DBG_ERROR, "badproto %d\n", proto);
            ret = -1;
        }
        break;

    /* Low-level port access */
    case XUM1541_GET_EOI:
        XUM_SET_STATUS_VAL(status, eoi ? 1 : 0);
        break;
    case XUM1541_CLEAR_EOI:
        eoi = 0;
        break;
    case XUM1541_IEC_WAIT:
        if (!cmds->cbm_wait(/*line*/request[1], /*state*/request[2])) {
            ret = 0;
            break;
        }
        /* FALLTHROUGH */
    case XUM1541_IEC_POLL:
        XUM_SET_STATUS_VAL(status, cmds->cbm_poll());
        DEBUGF(DBG_INFO, "poll=%x\n", XUM_GET_STATUS_VAL(status));
        break;
    case XUM1541_IEC_SETRELEASE:
        cmds->cbm_setrelease(/*set*/request[1], /*release*/request[2]);
        break;
    case XUM1541_PP_READ:
        // Disallow if in IEEE mode.
        if ((currState & XUM1541_IEEE488_PRESENT)) {
            ret = -1;
            break;
        }
        XUM_SET_STATUS_VAL(status, iec_pp_read());
        break;
    case XUM1541_PP_WRITE:
        // Disallow if in IEEE mode.
        if ((currState & XUM1541_IEEE488_PRESENT)) {
            ret = -1;
            break;
        }
        iec_pp_write(request[1]);
        break;
    case XUM1541_PARBURST_READ:
        // Disallow if in IEEE mode.
        if ((currState & XUM1541_IEEE488_PRESENT)) {
            ret = -1;
            break;
        }
        XUM_SET_STATUS_VAL(status, nib_parburst_read_checked());
        break;
    case XUM1541_PARBURST_WRITE:
        // Disallow if in IEEE mode.
        if ((currState & XUM1541_IEEE488_PRESENT)) {
            ret = -1;
            break;
        }
        // Sets suppressNibCmd if we're doing a read/write track.
        nib_parburst_write_checked(request[1]);
        break;
#ifdef SRQ_NIB_SUPPORT
    case XUM1541_SRQBURST_READ:
        // Disallow if in IEEE mode.
        if ((currState & XUM1541_IEEE488_PRESENT)) {
            ret = -1;
            break;
        }
        XUM_SET_STATUS_VAL(status, nib_srqburst_read_checked());
        break;
    case XUM1541_SRQBURST_WRITE:
        // Disallow if in IEEE mode.
        if ((currState & XUM1541_IEEE488_PRESENT)) {
            ret = -1;
            break;
        }
        // Sets suppressNibCmd if we're doing a read/write track.
        nib_srqburst_write_checked(request[1]);
        break;
#endif // SRQ_NIB_SUPPORT
#ifdef TAPE_SUPPORT
    case XUM1541_TAP_PREPARE_CAPTURE:
        XUM_SET_STATUS_VAL(status, Tape_PrepareCapture());
        break;
    case XUM1541_TAP_PREPARE_WRITE:
        XUM_SET_STATUS_VAL(status, Tape_PrepareWrite());
        break;
    case XUM1541_TAP_GET_SENSE:
        XUM_SET_STATUS_VAL(status, Tape_GetSense());
        break;
    case XUM1541_TAP_WAIT_FOR_STOP_SENSE:
        XUM_SET_STATUS_VAL(status, Tape_WaitForStopSense());
        break;
    case XUM1541_TAP_WAIT_FOR_PLAY_SENSE:
        XUM_SET_STATUS_VAL(status, Tape_WaitForPlaySense());
        break;
    case XUM1541_TAP_MOTOR_ON:
        XUM_SET_STATUS_VAL(status, Tape_MotorOn());
        break;
    case XUM1541_TAP_MOTOR_OFF:
        XUM_SET_STATUS_VAL(status, Tape_MotorOff());
        break;
    case XUM1541_TAP_GET_VER:
        XUM_SET_STATUS_VAL(status, Tape_GetTapeFirmwareVersion());
        break;
#endif // TAPE_SUPPORT
    default:
        DEBUGF(DBG_ERROR, "ERR: bulk cmd %d not impl.\n", cmd);
        ret = -1;
    }

    return ret;
}
