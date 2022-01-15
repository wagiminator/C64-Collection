# DiskBuddy64 - Connect your Commodore 1541 Floppy Disk Drive to a PC
DiskBuddy64 is a minimal adapter that can interface a Commodore 1541(II) floppy disk drive to your PC via USB in order to read from and write on disks. It uses its own fast loader to minimize the transfer times. The integrated USB to serial converter can also be used as a SerialUPDI programmer for the on-board ATtiny microcontroller, so that no additional hardware is required to flash the firmware. The DiskBuddy64 is controlled via a command line interface or a graphical front end written in Python.

![DiskBuddy64_pic5.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_DiskBuddy64/documentation/DiskBuddy64_pic5.jpg)

The DiskBuddy64 is a proprietary interface. It does not turn the floppy disk drive into a standard USB mass storage device. Instead, it offers the ability to send low-level IEC commands back and forth over USB. The provided Python scripts were developed to support this interface and can directly access the floppy disk drive and the contents of an inserted floppy disk. DiskBuddy64 cannot read or write copy-protected disks.

# Hardware
The schematic is shown below:

![DiskBuddy64_wiring.png](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_DiskBuddy64/documentation/DiskBuddy64_wiring.png)

## Microcontroller
One of the (not quite so) new 8-pin [tinyAVRs](https://ww1.microchip.com/downloads/en/DeviceDoc/ATtiny202-204-402-404-406-DataSheet-DS40002318A.pdf) was chosen for this project. Any of these MCUs can be used: ATtiny202, ATtiny212, ATtiny402 or ATtiny412. Don't buy them in China, they are much cheaper e.g. at [Digikey](https://www.digikey.de/de/products/detail/microchip-technology/ATTINY202-SSN/9947534) (€0.45).

## USB to Serial Converter
The [CH340N](https://datasheet.lcsc.com/lcsc/2101130932_WCH-Jiangsu-Qin-Heng-CH340N_C506813.pdf) (or CH330N) is now in use everywhere with me. At around €0.45 (if you buy [10 pieces](https://lcsc.com/product-detail/USB-ICs_WCH-Jiangsu-Qin-Heng-CH340N_C506813.html)) it is very cheap, fast, small, uncomplicated, reliable and hardly requires any external components (not even a crystal). It's probably the best choice for this small project too. And it is also used as a SerialUPDI programmer for the ATtiny.

## IEC Connector
The floppy disk drive is connected to the adapter via the [6-pin DIN jack](https://aliexpress.com/item/1005001265863686.html). Alternatively, a serial cable can be soldered directly to the corresponding pads.

![DiskBuddy64_pic1.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_DiskBuddy64/documentation/DiskBuddy64_pic1.jpg)

# Software
## Floppy Disk Drive
The main reason for the low speed of the 1541 floppy disk drive is the slow serial data transfer to the computer. As with most fast loaders, a proprietary transmission protocol is used here to accelerate this. Instead of the synchronous serial protocol with handshake and lots of waiting times, an asynchronous 2-bit parallel data transmission is used. For this to work, the sender and receiver must understand this protocol. For this purpose, a corresponding program is loaded into the RAM area of the memory and executed on the floppy disk drive side. The corresponding source codes (in 6502 assembly) can be found in the software/drive/ folder. For a more detailed explanation of the functional principle, I recommend Michael Steil's [website](https://www.pagetable.com/?p=568). Take the opportunity to watch his [Ultimate Commodore 1541 Drive Talk](https://youtu.be/_1jXExwse08), then you will know (almost) everything you need to know about the Commodore floppy disk drives.

## DiskBuddy64 Adapter
The DiskBuddy64 adapter bridges the gap between your PC and the Commodore 1541 floppy disk drives. It communicates with the floppy disk drive via bit banging of the IEC protocol and with the PC via the integrated USB-to-serial converter. The adapter also supports the proprietary protocol for faster data transmission. It essentially provides the basic functions of the IEC protocol in such a way that they can be called up from the PC using serial commands via USB. The source codes of the firmware can be found in the software/avr/ folder.

## PC
The Python scripts provide the user interface on the PC. These control the adapter and thus the floppy disk drive via USB. Both command line tools and a simple graphical user interface are available. The Python scripts can be found in the software/pc/ folder.

## D64 File Format
DiskBuddy64 uses the D64 format to read or create disk images. The structure of a D64 file can be found [here](http://unusedino.de/ec64/technical/formats/d64.html). The D64 format is supported by all C64 emulators as well as by [SD2IEC](https://github.com/wagiminator/C64-Collection/tree/master/C64_SD2IEC_LP) and Pi1541.

## Compiling and Uploading the Firmware
- Set the serial mode switch on the device to "UPDI". 
- Connect the device to a USB port of your PC.
- Use one of the following methods:

### If using the graphical front end (recommended)
- Execute the respective Python script on your PC: `python diskbuddy-gui.py`.
- Click on <kbd>Flash firmware</kbd>.

### If using the command line tool
- Execute the respective Python script on your PC: `python flash-firmware.py`.

### If using the Arduino IDE
- Open your Arduino IDE.
- Make sure you have installed [megaTinyCore](https://github.com/SpenceKonde/megaTinyCore).
- Go to **Tools -> Board -> megaTinyCore** and select **ATtiny412/402/212/202**.
- Go to **Tools** and choose the following board options:
  - **Chip:**           Select the chip you have installed on your device
  - **Clock:**          16 MHz internal
  - **Programmer:**     SerialUPDI
  - Leave the rest at the default settings.
- Go to **Tools -> Burn Bootloader** to burn the fuses.
- Open the TapeBuddy64 sketch and click **Upload**.

### If using the makefile (Linux/Mac)
- Download [AVR 8-bit Toolchain](https://www.microchip.com/mplab/avr-support/avr-and-arm-toolchains-c-compilers) and extract the sub-folders (avr, bin, include, ...) to /software/tools/avr-gcc. To do this, you have to register for free with Microchip on the download site.
- Open a terminal.
- Navigate to the folder with the makefile and the Arduino sketch.
- Run `DEVICE=attiny202 make install` to compile, burn the fuses and upload the firmware (change DEVICE accordingly).

## Installing Python and Drivers
Python needs to be installed on your PC in order to use the software. Most Linux distributions already include this. Windows users can follow these [instructions](https://www.pythontutorial.net/getting-started/install-python/). In addition PySerial and Tkinter (8.6 or newer) must be installed. However, these are already included in most Python installations.

Windows users may also need to install a [driver](http://www.wch.cn/download/CH341SER_ZIP.html) for the CH340N USB to serial adapter. This is not necessary for Linux or Mac users.

# Operating Instructions
## Preparation
- Set the serial mode switch on your DiskBuddy64 adapter to "UART".
- Connect the adapter to your Commodore 1541(II) floppy disk drive via an IEC cable.
- Connect the adapter to your PC via a USB cable.
- Switch on your floppy disk drive.
- Use the provided Python scripts in the software/pc/ folder on your PC.

## Command Line Interface
The following command line tools are available:

### Reading status of connected IEC devices
`python disk-status.py`

### Reading directory of an inserted floppy disk
```
python disk-dir.py [-h] [-d {8,9,10,11}]

optional arguments:
-h, --help                  show help message and exit
-d, --device                device number of disk drive (8-11, default=8)
```

### Formatting an inserted floppy disk
```
python disk-format.py [-h] [-x] [-n] [-m] [-d {8,9,10,11}] [-t TITLE] [-i IDENT]

optional arguments:
-h, --help                  show help message and exit
-x, --extend                format 40 tracks
-n, --nobump                do not bump head
-m, --demagnetize           demagnetize disk (recommended for new disks)
-d, --device                device number of disk drive (8-11, default=8)
-t TITLE, --title TITLE     disk title (max 16 characters, default=commodore)
-i IDENT, --ident IDENT     disk ident (2 characters, default=64)

Example: python disk-format.py -t games -i a7
```

### Reading from floppy disk to a D64 image file
```
python disk-read.py [-h] [-b] [-d {8,9,10,11}] [-f FILE]

optional arguments:
-h, --help            show help message and exit
-b, --bamonly         only read blocks with BAM entry (recommended)
-d, --device          device number of disk drive (8-11, default=8)
-f FILE, --file FILE  output file (default=output.d64)

Example: python disk-read.py -b -f game.d64
```

### Writing a D64 image file to floppy disk
```
python disk-write.py [-h] [-b] [-d {8,9,10,11}] -f FILE

optional arguments:
-h, --help            show help message and exit
-b, --bamonly         only write blocks with BAM entry (recommended)
-d, --device          device number of disk drive (8-11, default=8)
-f FILE, --file FILE  input file (*.d64)

Example: python disk-write.py -b -f game.d64
```

### Comparing D64 image file with floppy disk content (verify)
```
python disk-verify.py [-h] [-b] [-d {8,9,10,11}] -f FILE

optional arguments:
-h, --help            show help message and exit
-b, --bamonly         only verify blocks with BAM entry (recommended)
-d, --device          device number of disk drive (8-11, default=8)
-f FILE, --file FILE  d64 file to compare the disk with

Example: python disk-verify.py -b -f game.d64
```

### Flashing firmware to DiskBuddy64 adapter
`python flash-firmware.py`

## Graphical Front End
- Execute the respective Python script on your PC: `python diskbuddy-gui.py`.
- The rest should be self explanatory.

![DiskBuddy64_gui2.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_DiskBuddy64/documentation/DiskBuddy64_gui2.jpg)
![DiskBuddy64_gui1.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_DiskBuddy64/documentation/DiskBuddy64_gui1.jpg)

## Working with D64 Files
Coming soon ...

# References, Links and Notes
1. [SD2IEC Project](https://github.com/wagiminator/C64-Collection/tree/master/C64_SD2IEC_LP)
2. [XU1541 Project](https://github.com/wagiminator/C64-Collection/tree/master/C64_xu1541)
3. [Michael Steil: Commodore Peripheral Bus](https://www.pagetable.com/?p=1018)
4. [Michael Steil: A 256 Byte Autostart Fast Loader](https://www.pagetable.com/?p=568)
5. [Michael Steil: Ultimate Commodore 1541 Drive Talk](https://youtu.be/_1jXExwse08)
6. [Jim Butterfield: How The VIC/64 Serial Bus Works](http://www.zimmers.net/anonftp/pub/cbm/programming/serial-bus.txt)
7. [J. Derogee: IEC disected](http://www.zimmers.net/anonftp/pub/cbm/programming/serial-bus.pdf)
8. [Immers/Neufeld/Moser: D64 File Format](http://unusedino.de/ec64/technical/formats/d64.html)
9. [OpenCBM Software](https://github.com/OpenCBM/OpenCBM)
10. [Microchip: ATtiny Datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/ATtiny202-204-402-404-406-DataSheet-DS40002318A.pdf)
11. [WCH: CH340N Datasheet](https://datasheet.lcsc.com/lcsc/2101130932_WCH-Jiangsu-Qin-Heng-CH340N_C506813.pdf)

![DiskBuddy64_pic2.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_DiskBuddy64/documentation/DiskBuddy64_pic2.jpg)
![DiskBuddy64_pic3.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_DiskBuddy64/documentation/DiskBuddy64_pic3.jpg)

# License
![license.png](https://i.creativecommons.org/l/by-sa/3.0/88x31.png)

This work is licensed under Creative Commons Attribution-ShareAlike 3.0 Unported License. 
(http://creativecommons.org/licenses/by-sa/3.0/)
