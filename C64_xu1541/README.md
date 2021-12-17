# xu1541 Commodore Floppy Disk Drive Adapter
xu1541 is a Commodore floppy disk drive to USB adapter based on the design by Till Harbaum and [Spiro R. Trikaliotis](https://spiro.trikaliotis.net/xu1541). It connects a variety of CBM drives (1541, 1541-II, 1570, 1571, 1581, SX-64's internal floppy disk) to USB-equipped PCs running Windows, Linux or Mac OS X. This allows for easy transfer of disk images to and from these devices and helps you preserve your old data.

![C64_xu1541_pic1.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_xu1541/documentation/C64_xu1541_pic1.jpg)

The xu1541 is a proprietary interface. It does not turn the floppy disk drive into a standard USB mass storage device. Instead, it offers the ability to send low-level IEC commands back and forth over the USB. [PC software](https://opencbm.trikaliotis.net/) that was developed to support this interface can then directly access the floppy disk drive and the contents of an inserted floppy disk.

# Installing the Firmware
An AVR programmer is required to flash the bootloader for the first time. A cheap and reliable programmer is e.g. [USBasp](https://aliexpress.com/wholesale?SearchText=usbasp). 

- Connect the programmer to the PC via USB and to the ICSP header on the board via an ICSP cable.
- Make sure you have installed [avrdude](https://learn.adafruit.com/usbtinyisp/avrdude).
- Open a terminal.
- Navigate to the software folder with the hex-files.
- Execute the following command (if necessary replace "usbasp" with the programmer you use):
  ```
  avrdude -c usbasp -p m8 -U lfuse:w:0x9f:m -U hfuse:w:0xc8:m -U flash:w:firmware_bootloader-usbtiny.hex
  ```
  
Since bootloader and firmware were flashed together, the xu1541 is now ready for use. Future firmware updates can be made via USB thanks to the bootloader. Use the pre-compiled program xu1541_update for this. Source Code und updates can be found on the [github page](https://github.com/OpenCBM/OpenCBM/tree/master/xu1541) of the original project.

In order to be able to use xu1541, [OpenCBM](https://opencbm.trikaliotis.net/opencbm-9.html) must be installed on your PC.

# Operating Instructions
- Connect your Commodore floppy disk drive via an IEC cable to your xu1541.
- Connect your xu1541 via a USB cable to your PC.
- Turn on your floppy disk drive.
- Open a terminal.
- Control your floppy disk drive via the OpenCBM [commands](https://opencbm.trikaliotis.net/opencbm-16.html). Examples:

|Function|Command|
|:-|:-|
|Test device|cbmctrl detect|
|Show directory of disk in drive 8|cbmctrl dir 8|
|Format disk in drive 8|cbmformat 8 GAMES,42|
|Format with 40 tracks|cbmforng -c -x 8 SOFTWARE,24|
|Copy D64 image to disk|d64copy --transfer=original "image.d64" 8|
|Copy from disk to D64 image|d64copy --transfer=original 8 "foo.d64"|
|Copy file from disk to PRG|cbmcopy --quiet --no-progress --transfer=original -r 8 "foo" -output="foo.prg"|

Do NOT connect the xu1541 to your Commodore C64!

# References, Links and Notes
1. [Original Project](https://spiro.trikaliotis.net/xu1541)
2. [xu1541 Firmware](https://github.com/OpenCBM/OpenCBM/tree/master/xu1541)
3. [OpenCBM User Guide](https://opencbm.trikaliotis.net/)
4. [OpenCBM Gitub Page](https://github.com/OpenCBM/OpenCBM)
