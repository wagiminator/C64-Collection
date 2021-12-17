/*
 * Main loop for at90usb-based devices
 * Copyright (c) 2009-2010 Nate Lawson <nate@root.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#include <avr/io.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <string.h>

#include "xum1541.h"

// Flag indicating we should abort any in-progress data transfers
volatile bool doDeviceReset;

// Flag for whether we are in EOI state
volatile uint8_t eoi;

// Are we in an active state? If so, run the command loop.
volatile bool device_running;

// Is the USB bus connected? If so, wait to enter active state.
static volatile bool usb_connected;

static bool USB_BulkWorker(void);

int
main(void)
{
    /*
     * Setup the CPU and USB configuration. Wait after power-on for VBUS.
     * This is to handle the case where we are being powered from the
     * IEC bus through the IOs without a USB connection. Nate analyzed the
     * time we run until brownout here and it defaults to 65 ms due to the
     * CKSEL/SUT fuse.
     *
     * Instead of just a delay, we can wait for VBUS to be active and then
     * be sure USB is connected. This works even when we can't use the
     * brown-out detector. On the USBKEY, the BOD can only be 2.6V or 3.4V
     * but it runs at 3.3V.
     */
    cpu_init();
    USB_Init();
    while (!usb_connected)
        wdt_reset();

    // Indicate device not ready
    board_init();
    board_set_status(STATUS_INIT);

    // If a CBM 153x tape drive is attached, detect it and enable tape mode.
    // If any IEC/IEEE drives are attached, detect them early.
    // This leaves the IO pins in the proper state to allow them to come
    // out of reset until we're ready to access them.
    cbm_init();

    /*
     * Process bulk transactions as they appear. Control requests are
     * handled separately via IRQs.
     */
    for (;;) {
        wdt_reset();

        while (device_running) {
            // Check for and process any commands coming in on the bulk pipe.
            USB_BulkWorker();

            /*
             * Do periodic tasks each command. If we found the device was in
             * a stalled state, reset it before the next command.
             */
            if (!TimerWorker())
                doDeviceReset = false;
        }

        // TODO: save power here when device is not running

        // Wait for device to be reconnected to USB
        while (!usb_connected)
            wdt_reset();
    }
}

void
EVENT_USB_Device_Connect(void)
{
    DEBUGF(DBG_ALL, "usbcon\n");
    board_set_status(STATUS_CONNECTING);
    doDeviceReset = false;
    usb_connected = true;
}

void
EVENT_USB_Device_Disconnect(void)
{
    DEBUGF(DBG_ALL, "usbdiscon\n");

    // Halt the main() command loop and indicate we are not configured
    usb_connected = false;
    device_running = false;
    board_set_status(STATUS_INIT);
}

void
EVENT_USB_Device_ConfigurationChanged(void)
{
    DEBUGF(DBG_ALL, "usbconfchg\n");

    // Clear out any old configuration before allocating
    USB_ResetConfig();

    /*
     * Setup and enable the two bulk endpoints. This must be done in
     * increasing order of endpoints (3, 4) to avoid fragmentation of
     * the USB RAM.
     */
    Endpoint_ConfigureEndpoint(XUM_BULK_IN_ENDPOINT, EP_TYPE_BULK,
        ENDPOINT_DIR_IN, XUM_ENDPOINT_BULK_SIZE, ENDPOINT_BANK_DOUBLE);
    Endpoint_ConfigureEndpoint(XUM_BULK_OUT_ENDPOINT, EP_TYPE_BULK,
        ENDPOINT_DIR_OUT, XUM_ENDPOINT_BULK_SIZE, ENDPOINT_BANK_DOUBLE);

    // Indicate USB connected and ready to start event loop in main()
    board_set_status(STATUS_READY);
    device_running = true;
}

void
EVENT_USB_Device_UnhandledControlRequest(void)
{
    uint8_t replyBuf[XUM_DEVINFO_SIZE];
    int8_t len;

    /*
     * Ignore non-class requests. We also only handle commands
     * that don't transfer any data or just transfer it into the host.
     */
    if ((USB_ControlRequest.bmRequestType & CONTROL_REQTYPE_TYPE) !=
        REQTYPE_CLASS) {
        DEBUGF(DBG_ERROR, "bad ctrl req %x\n",
            USB_ControlRequest.bmRequestType);
        return;
    }

    // Process the command and get any returned data
    memset(replyBuf, 0, sizeof(replyBuf));
    len = usbHandleControl(USB_ControlRequest.bRequest, replyBuf);
    if (len == -1) {
        DEBUGF(DBG_ERROR, "ctrl req err\n");
        board_set_status(STATUS_ERROR);
        return;
    }

    // Control request was handled so ack it to allow another
    Endpoint_ClearSETUP();

    // Send data back to host and finalize the status phase
    if ((USB_ControlRequest.bmRequestType & CONTROL_REQTYPE_DIRECTION) ==
        REQDIR_DEVICETOHOST) {
        Endpoint_Write_Control_Stream_LE(replyBuf, len);
        Endpoint_ClearOUT();
    } else {
        while (!Endpoint_IsINReady())
            ;
        Endpoint_ClearIN();
    }
}

static bool
USB_BulkWorker()
{
    uint8_t cmdBuf[XUM_CMDBUF_SIZE], statusBuf[XUM_STATUSBUF_SIZE];
    int8_t status;

    /*
     * If we are not connected to the host or a command has not yet
     * been sent, no more processing is required.
     */
    if (USB_DeviceState != DEVICE_STATE_Configured)
        return false;
    Endpoint_SelectEndpoint(XUM_BULK_OUT_ENDPOINT);
    if (!Endpoint_IsReadWriteAllowed())
        return false;

#ifdef DEBUG
    // Dump the status of both endpoints before getting the command
    Endpoint_SelectEndpoint(XUM_BULK_IN_ENDPOINT);
    DEBUGF(DBG_INFO, "bsti %x %x %x %x %x %x %x %x\n",
        Endpoint_GetCurrentEndpoint(),
        Endpoint_BytesInEndpoint(), Endpoint_IsEnabled(),
        Endpoint_IsReadWriteAllowed(), Endpoint_IsConfigured(),
        Endpoint_IsINReady(), Endpoint_IsOUTReceived(), Endpoint_IsStalled());
    Endpoint_SelectEndpoint(XUM_BULK_OUT_ENDPOINT);
    DEBUGF(DBG_INFO, "bsto %x %x %x %x %x %x %x %x\n",
        Endpoint_GetCurrentEndpoint(),
        Endpoint_BytesInEndpoint(), Endpoint_IsEnabled(),
        Endpoint_IsReadWriteAllowed(), Endpoint_IsConfigured(),
        Endpoint_IsINReady(), Endpoint_IsOUTReceived(), Endpoint_IsStalled());
#endif

    // Read in the command from the host now that one is ready.
    if (!USB_ReadBlock(cmdBuf, sizeof(cmdBuf))) {
        board_set_status(STATUS_ERROR);
        return false;
    }

    // Allow commands to leave the extended status untouched
    memset(statusBuf, 0, sizeof(statusBuf));

    /*
     * Decode and process the command.
     * usbHandleBulk() stores its extended result in the output buffer,
     * up to XUM_STATUSBUF_SIZE.
     *
     * Return values:
     *   >0: completed ok, send the return value and extended status
     *    0: completed ok, don't send any status
     *   -1: error, no status
     */
    status = usbHandleBulk(cmdBuf, statusBuf);
    if (status > 0) {
        statusBuf[0] = status;
        USB_WriteBlock(statusBuf, sizeof(statusBuf));
    } else if (status < 0) {
        DEBUGF(DBG_ERROR, "usbblk err\n");
        board_set_status(STATUS_ERROR);
        Endpoint_StallTransaction();
        return false;
    }

    return true;
}

/*
 * Stall all endpoints and set a flag indicating any current transfers
 * should be aborted. IO loops will see this flag in TimerWorker().
 */
void
SetAbortState()
{
    uint8_t origEndpoint = Endpoint_GetCurrentEndpoint();

    doDeviceReset = true;
    Endpoint_SelectEndpoint(XUM_BULK_OUT_ENDPOINT);
    Endpoint_StallTransaction();
    Endpoint_SelectEndpoint(XUM_BULK_IN_ENDPOINT);
    Endpoint_StallTransaction();

    Endpoint_SelectEndpoint(origEndpoint);
}

/*
 * Periodic maintenance task. This code can be called at any point, but
 * at least needs to be called enough to reset the watchdog.
 *
 * If the board's timer has triggered, we also update the board's display
 * or any other functions it does when the timer expires.
 */
bool
TimerWorker()
{
    wdt_reset();

    // Inform the caller to quit the current transfer if we're resetting.
    if (doDeviceReset)
        return false;

    // If the timer has fired, update the board display
    if (board_timer_fired())
        board_update_display();
    return true;
}

/*
 * The Linux and OSX call the configuration changed entry each time
 * a transaction is started (e.g., multiple runs of cbmctrl status).
 * We need to reset the endpoints before reconfiguring them, otherwise
 * we get a hang the second time through.
 *
 * We keep the original endpoint selected after returning.
 */
void
USB_ResetConfig()
{
    static uint8_t endpoints[] = {
        XUM_BULK_IN_ENDPOINT, XUM_BULK_OUT_ENDPOINT, 0,
    };
    uint8_t lastEndpoint, *endp;

    lastEndpoint = Endpoint_GetCurrentEndpoint();

    for (endp = endpoints; *endp != 0; endp++) {
        Endpoint_SelectEndpoint(*endp);
        Endpoint_ResetFIFO(*endp);
        Endpoint_ResetDataToggle();
        if (Endpoint_IsStalled())
            Endpoint_ClearStall();
    }

    Endpoint_SelectEndpoint(lastEndpoint);
}

/*
 * Read a block from the host's OUT endpoint, handling aborts.
 */
bool
USB_ReadBlock(uint8_t *buf, uint8_t len)
{
    // Get the requested data from the host
    Endpoint_SelectEndpoint(XUM_BULK_OUT_ENDPOINT);
    Endpoint_Read_Stream_LE(buf, len, AbortOnReset);

    // Check if the current command is being aborted by the host
    if (doDeviceReset)
        return false;

    Endpoint_ClearOUT();
    return true;
}

/*
 * Send a block to the host's IN endpoint, handling aborts.
 */
bool
USB_WriteBlock(uint8_t *buf, uint8_t len)
{
    // Get the requested data from the host
    Endpoint_SelectEndpoint(XUM_BULK_IN_ENDPOINT);
    Endpoint_Write_Stream_LE(buf, len, AbortOnReset);

    // Check if the current command is being aborted by the host
    if (doDeviceReset)
        return false;

    Endpoint_ClearIN();
    return true;
}

/*
 * Callback for the Endpoint_Read/Write_Stream functions. We abort the
 * current stream transfer if the user sent a reset message to the
 * control endpoint.
 */
uint8_t
AbortOnReset()
{
    return doDeviceReset ? STREAMCALLBACK_Abort : STREAMCALLBACK_Continue;
}
