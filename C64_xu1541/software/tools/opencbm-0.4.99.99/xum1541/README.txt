xum1541 firmware
================

The xum1541 is firmware for USB devices that connect a 15x1 drive to
your PC, notably the ZoomFloppy. It is based on the Atmel AT90USB family
of microcontrollers and is provided under the GPL license.

For more information, see the ZoomFloppy web page at:

    http://go4retro.com/

Or the xum1541 web page:

    http://www.root.org/~nate/c64/xum1541/


Revisions
=========
0.7 (2011/5/10) - Add IEEE-488 support (thanks to Tommy Winkler).
0.6 (2010/7/5) - New protocol (version 6) with reduced latency and
    support for indefinite waiting, better reset when the previous command
    is aborted with ^C, new ZoomFloppy board design, updated to LUFA 091223
0.4 (2010/1/4) - Bugfixes for reset handling, both for IEC and USB.
    Protocol version 2.
0.3 (2009/12/23) - Add nibbler (mnib/nibtools) support and misc stability
    fixes. Protocol version 1.
0.2 (2009/12/11) - Initial beta release for AT90USBKEY board.
    Protocol version 0.


Installation
============
First, install OpenCBM. You'll need to enable the xum1541 plugin in
opencbm.conf. See the OpenCBM docs for more info on installation.

If you are on Windows, now you need to install the libusb driver.
Follow the steps to install the library in the xu1541/windrv directory.

If you purchased a ZoomFloppy, the device comes with the latest firmware
and you do not need to change it. Only do the following if you are upgrading
the firmware.

To install or upgrade the firmware on a device, you just need to flash it
with the .hex file from this distribution.  All AT90USB CPUs come with a
USB bootloader builtin. This means that you don't need any special cables
to flash the device. Simply plug it into your USB port and program it. I
recommend the free Atmel Flip or avrdude programming software.

The steps to program the firmware with Flip are:

1. Start up Atmel Flip
2. Select the appropriate device type (Device->Select). Choose one of
the following:
   - ZoomFloppy: ATmega32U2
   - USBKEY development board: AT90USB1287
   - Original Bumble-B: AT90USB162
3. Plug in the device via USB
4a. For most boards, run the included firmware update utility (called
"xum1541cfg") to put the device in bootloader mode.
4b. For the USBKEY, press and hold both the RST and HWB buttons at the same
time. Now, release the RST button and then the HWB button. This puts the device
in upgrade mode. There's no need to hurry, just make sure both buttons
are held down at the same time and then released in the right order.
5. Click on the USB cable icon and select "USB" then "open". If all goes
well, the lower right corner will say "USB ON". If it can't connect to
the device, you'll see "Could not open USB device". In this case, repeat
the steps above in #3. Be sure you're releasing the buttons in the right
order.
6. File->Load Hex File and choose the firmware .hex file named
xum1541-(board)-(version).hex. Be sure to pick the right board type:
   - ZOOMFLOPPY
   - USBKEY
   - BUMBLEB
Flip should say "HEX file parsed" in the lower left of the window status area.
7. Be sure the "Operations Flow" checkboxes are set to Erase, Program, and
Verify.
8. Click the Run button
9. If all goes well, the lower left status area will say "Verify PASS".
If there were any problems, do not unplug the device. Select the appropriate
firmware and settings and click "Run" again.

You're done! Now unplug and replug in the xum1541 and verify it is present
by running "cbmctrl detect". You should see something like:

    8: 1540 or 1541 (XP1541)


Compiling
=========
Whenever new releases come out, the .hex files are updated as well.
Thus most users should never need to compile the firmware. If you're a
developer, here's how to do it.

Get avr-gcc, avr-binutils, avr-libc, and a Unix shell environment (make).
On Windows, I use WinAVR to get all of the AVR utils in one place and
Cygwin for the shell environment. Try just typing "make" and it should
build the firmware. To enable the debug build, uncomment the appropriate
line in the Makefile. If the build fails, check your path to be sure
the AVR bin directory is present.

Currently I am building releases using WinAVR-20100110. The LUFA version
included in this distribution is 091223.


Usage notes
===========
Please note that the s2 and nib protocols do not work if multiple drives
are connected and powered on. This is because they toggle the ATN line,
which is only ok if there is no other drive on the bus. You should
power off and disconnect any drives that aren't in use when using s2
transfers or nibtools.

If using a Mac and you see multiple newlines after hitting ENTER to start
a command, you can workaround this by setting your keyboard "delay until
repeat" level to a slower value.

IEEE-488 support has been added (SFD-1001, CBM-8050, CBM-4040 drives).
Currently, only cbmctrl is supported although file and image copy
(cbmread/cbmwrite and d82copy) should be implemented in the future.
It is possible to transfer files to/from the floppy with just cbmctrl.

CAUTION: never connect serial or parallel IEC drives at the same time as
IEEE-488 drives. You should only use one bus type at a time.


Developer notes
===============
The xum1541 is very different from the xu1541 (e.g., the USB IO model).
However, the IEC routines are based on code from the xu1541 and I got a
lot of ideas from Till Harbaum's design. The LUFA USB library by Dean
Camera was also invaluable.

The xum1541 has 3 USB endpoints: control, bulk in, and bulk out.
The control endpoint is used for initializing the device and reseting it
and the CBM drive if an error occurs. It is run from an interrupt handler
so that the command can override any pending transfer.

The bulk endpoints are used for the command, data, and status phases of
various transactions. The ordinary procedure for a command that transfers
data from the drive is:

1. Write 4-byte command descriptor to bulk out pipe
2. Wait indefinitely on bulk in pipe for data to be transferred.
   Once it is ready, keep reading until all has been retrieved.
3. Wait indefinitely on bulk in pipe for 3-byte status to be transferred.
   The status phase is optional for some commands.

The procedure for a command that transfers data to the drive is:
1. Write 4-byte command descriptor to bulk out pipe
2. Write data to the bulk out pipe until all has been transferred.
3. Wait indefinitely on bulk in pipe for 3-byte status to be transferred.
   The status phase is optional for some commands.

The xu1541 uses only control transfers, and thus has to implement IO in
two stages. First it transfers data to the microcontroller in a 128-byte
buffer, then it transfers it to the PC or drive. We do not use this model.
Since the AT90USB can stream the data all at once, we transfer each byte
as it is ready (e.g., usbSendByte() and usbRecvByte()). This increases
performance and allows support for the nibtools protocol for copying
protected disks.

The firmware is organized in a logical way to support multiple device
models being based off the same firmware. The first model is the USBKEY,
which is based on the Atmel development board of the same name. The CPU
files (e.g., cpu-usbkey.h) define routines that are unique to the CPU, in
this case the AT90USB1287. This includes timers and initialization. The
board files (e.g., board-usbkey.[ch]) define the methods for setting up
and interacting with the IO pins on the board. Each device is composed of
a combination of board/cpu in the Makefile.

This approach allows reuse. Adding a new design is simply a matter of
making a copy of the board and cpu files and adding your own interface
routines. However, you should avoid doing this whenever possible.
For example, if you changed to an AT90USB16 CPU from the AT90USB1287, there
is not any need yet to change CPU files as both use the same IO ports and
same basic timer routines.


ZoomFloppy model
================
The ZoomFloppy is a simpler version of the original design, intended for
low-cost manufacturing with high-speed performance. It is available
commercially from RETRO Innovations, making it the best choice for most
users.

    http://go4retro.com/

If you want to build it yourself, it can also be based on the Bumble-B
daughterboard. However, the easiest option for DIY is the USBKEY board
(below) since that only requires soldering a single connector (DB25) to
the development board.

This device uses an ATmega32U2 microcontroller (AT90USB162 if you use the
original Bumble-B). It has a 7406N hex inverter for better control of the
pins. It runs at 5V with the board supplying power for the inverter.

For build info, see the included schematic, zoomfloppy-schem-*.png.


USBKEY model
============
This is the first generation board and is based on the Atmel AT90USBKEY
developer's kit. The ports are allocated as follows (see board-usbkey.c):

  A: CBM IEC lines, in and out
  C: CBM parallel lines, bidirectional
  D: UART for debug output (optional, under #ifdef DEBUG)

We use these lines to interface with an existing XAP1541 adapter via the
standard DB25 printer port. Thus, the prototype requires you to own an
XA/XAP1541, which is connected to the USBKEY via a female DB25 port that
is directly wired to the board. Alternatively, the XAP1541 IEC and CBM
parallel port, transistors, etc. can be built into a daugterboard that is
plugged into the USBKEY. The xum1541 firmware cannot tell the difference
between these two designs as they behave identically.

Pinouts:
The config is wired for an XAP1541 adapter, meaning logic lines are
inverted for driving the IEC bus (drive 1 out the IO port to pull
the corresponding IEC line low). Inputs are read normally. Port A is next
to the Atmel logo, port C is the next one to the right.

Port A (IEC):
1  nc
2  GND
3  RST in
4  RST out
5  DATA in
6  DATA out
7  CLK in
8  CLK out
9  ATN in
10 ATN out

Port C (parallel):
1  nc
2  GND
3  data7
4  data6
5  data5
6  data4
7  data3
8  data2
9  data1
10 data0


Other models
============
I expect others will offer custom or prepackaged boards based on this
firmware.


Tasks
=====
Bugs:
- Update IEC read routine timing
- Figure out which loops don't check Timer abort routine yet.
  Add Timer checking back into nib routines.
- Star Commander needs testing as it may not work with the xum1541
- The USBKEY firmware fails to enter bootloader mode from software.
  This will eventually be fixed but is not a big deal since that board
  has hardware buttons to do the same function. The ZoomFloppy works fine.

Improvements:
- Add SRQ nibbling support
- Update firmware utility to do DFU and check device version/type so it is
  impossible to write the wrong firmware and easier for users to upgrade.
- Integrate Teensy support, factor out timer routines to common file
- Add support to program in a serial number to EEPROM
- Improve LEDs, especially on USBKEY (which has 4)
- Debug printing via the UART is not supported on ZoomFloppy since it has
  to use this pin for the IEC DATA connection. Another route for debugging
  should be found for it.
- Test and update to libusb 1.0 since 0.x is no longer supported.
- Test various cable lengths and USB hubs to be sure it is reliable
- Heavy system activity can sometimes disrupt transfers, especially timing
  sensitive operations like nibbling. Make the firmware more resistant to
  intermittent hangs.

General opencbm issues not in xum1541 code:
- cbmcopy only does byte at a time, does not use read_n() API from plugin.
  Thus, reading/writing files from a disk with cbmcopy will be slow and not
  take advantage of the speed of the xum1541. However, d64copy and nibtools
  can perform at full speed since they use a separate IO route.

General opencbm issues for a 0.5.x release:
- instcbm (Windows):
  * add and debug plugin functionality (partly there, but not in CVS yet)
  * --nocopy support (partly in local sandbox, but not in CVS yet)
    . works for all but opencbm.conf, which still resides in
      WINDOWS\System32 dir
    . Partial remove (only one or more plugins)
    . if the default plugin is removed, make another one the default
    . remove the plugin from the configuration file
    . only remove OpenCBM if all plugins are removed
    . Remove opencbm.conf if OpenCBM is removed completely
  * re-add --update, --check and --debug
  * change defaults only if plugin install was successfull
  * add --purge option for --remove to completely remove OpenCBM from machine

- Document new instcbm functionality
- Document changes
- Remove \todo doxygen comments and replace it with "correct" comments.
