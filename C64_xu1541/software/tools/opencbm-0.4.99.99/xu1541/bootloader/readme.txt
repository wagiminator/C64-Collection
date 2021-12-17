xu1541 'BIOS' bootloader
------------------------

Starting from the early adoption of Thomas Fischl's bootloader
it now got added some functionality so that decent functions of
the bootloader/BIOS can be used from application firmware.
To reflect this capabilities the BIOS carries own version
information. As with the xu1541 firmware, odd major version
numbers denote an AVRUSB stack, even major versions tell that
USBTiny is used.


Usage scenario
--------------

The recently added main functionality for the new BIOS structure
is that the xu1541 firmware can be upgraded without explicitly
setting the xu1541 device into bootloader mode by unplugging the
device, setting a jumper and replugging it to the USB bus (see
the dedicated readme file for xu1541_update).


Upgrading the bootloader and firmware
-------------------------------------

The following 'quick install' description is intended for
developers and requires basic knowledge about compiling in
general and working with micrcontroller devices.

   I. It is important to recompile and install the latest
      versions of the tools for the xu1541 device like
      xu1541_update. The xu1541 project toolchain needed to be
      updated, so that the extended capabilities of the new BIOS
      can be detected and used.

  II. Compile the bootloader (version 1.5/2.5) or take the
      precompiled *.hex binary files and initially program (or
      reprogram) your xu1541 with a typical ISP cable or an
      ISP programmer. The bootloader makefile contains a
      predefined target for using avrdude with a LPT port based
      ISP cable, e.g. do:

         make program -f Makefile-avrusb

      Editing the CABLE variable enables other options like
      using Thomas Fischl's USBasp ISP programmer.

 III. From the previous step, the xu1541 is not only connected
      to the ISP programming cable, but to the USB bus too to
      become powered -- this does not hold true if using a
      dedicated ISP programmer, but for LTP based ISP cables
      for example.
      You may leave the xu1541 device connected to the USB bus,
      the previous programming step ensured that the device was
      reset and is now directly ready for getting updated with
      further firmware.
      Call the compiled version of the update tool from step I,
      xu1541_update, and program the device with the new
      firmware:

          xu1541_update.exe firmware/firmware-avrusb.hex


At any later time you may update the firmware with new version
by entering step III only. With the new bootloader BIOS, a
firmware version making (some) use of that BIOS and the recent
version of xu1541_update, you are always able to reprogram the
device at any time and without setting jumpers.

xu1541_update calls the device firmware to shut down normal
operation and entering bootloader mode (this is signalled with
the yellow LED going on and staying on). After that it updates
the firmware (currently the bootloader is not able to update
itself) and then resets the device, so that the new firmware
gets activated.
If the firmware cannot be started for any reason, the
bootloader gets activated again so that the xu1541 device can
be reprogrammed again. If for any reason the xu1541 fails to
start up into an operational state, there's still the option to
immediately put the xu1541 into bootloader mode with the help
of the jumper switch between pins 9 and 10 of SV2. Refer to the
xu1541_update readme for further instructions.
