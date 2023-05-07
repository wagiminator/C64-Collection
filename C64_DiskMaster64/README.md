# DiskMaster64 - Connect your Commodore 1541 Floppy Disk Drive to a PC
The DiskMaster64 is a small adapter that allows you to connect a Commodore 1541(II) floppy disk drive to your computer via USB. This enables you to read from and write to floppy disks. The adapter is equipped with a fast loading system that reduces transfer times. You can control the DiskMaster64 using a command line interface or a graphical interface programmed in Python.

The DiskMaster64 is a proprietary interface and does not transform the floppy disk drive into a typical USB mass storage device. Instead, it allows you to communicate using low-level IEC commands over USB. Python scripts are provided to support this interface, enabling you to access the floppy disk drive and its contents directly. Thanks to the integrated fast loader, you can read or write floppy disks at 12 times the normal speed, allowing you to copy an entire disk in about 32 seconds. However, the DiskMaster64 cannot copy disks that are copy-protected.

The DiskMaster64 is a conversion of the [DiskBuddy64](https://github.com/wagiminator/C64-Collection/tree/master/C64_DiskBuddy64) from the ATtiny to the CH552 microcontroller. This makes the adapter even smaller, simpler, and cheaper while retaining its full functionality.

![DiskMaster64_pic1.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_DiskMaster64/documentation/DiskMaster64_pic1.jpg)

# Hardware
The schematic is shown below:

![DiskMaster64_wiring.png](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_DiskMaster64/documentation/DiskMaster64_wiring.png)

## CH552E 8-bit USB Device Microcontroller
The CH552E is a low-cost, enhanced E8051 core microcontroller compatible with the MCS51 instruction set. It has an integrated USB 2.0 controller with full-speed data transfer (12 Mbit/s) and supports up to 64 byte data packets with integrated FIFO and direct memory access (DMA). The CH552E has a factory built-in bootloader so firmware can be uploaded directly via USB without the need for an additional programming device.

## IEC Connector
To connect the floppy disk drive to the adapter, you need a 4-core shielded cable. One side of the cable is soldered to the appropriate pads on the adapter board, while the other side is soldered to a [6-pin DIN connector](https://aliexpress.com/wholesale?SearchText=6-pin+DIN+connector+male).

![DiskMaster64_pic2.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_DiskMaster64/documentation/DiskMaster64_pic2.jpg)

# Software
## Floppy Disk Drive
The slow serial data transfer to the computer is the main reason why the 1541 floppy disk drive is not the fastest out there. To speed up this process, a proprietary transmission protocol is used, which is common with most fast loaders. This protocol replaces the synchronous serial protocol that involves handshaking and a lot of waiting time. Instead, it uses an asynchronous 2-bit parallel data transmission that requires both the sender and receiver to understand the protocol. To achieve this, a specific program is loaded into the RAM of the memory and executed on the floppy disk drive side. The corresponding source codes, which are written in 6502 assembly, can be found in the software/drive/ folder.

To increase the speed even further, the GCR encoding and decoding is not done by the floppy disk drive, but by the PC. This means that reading or writing an entire 35-track floppy disk takes only 32 seconds, which is about 12 times faster than the normal speed. While it may not be a world record, it is still sufficient for most purposes.

For a more detailed explanation of some functional principle, I recommend Michael Steil's [website](https://www.pagetable.com/?p=568). Take the opportunity to watch his [Ultimate Commodore 1541 Drive Talk](https://youtu.be/_1jXExwse08), then you will know (almost) everything you need to know about the Commodore floppy disk drives.

## DiskMaster64 Adapter
The DiskMaster64 adapter connects your PC to the Commodore 1541 floppy disk drives. It communicates with the floppy disk drive by bit banging the IEC protocol and with the PC by using the USB-CDC serial interface. The adapter also supports a proprietary protocol to enable faster data transmission. Essentially, it offers the fundamental functions of the IEC protocol, which can be accessed from the PC using serial commands through USB. The source codes of the firmware can be found in the software/avr/ folder.

## PC
The Python scripts provide the user interface on the PC. These control the adapter and thus the floppy disk drive via USB. Both command line tools and a simple graphical user interface are available. The Python scripts can be found in the software/pc/ folder.

## D64 File Format
DiskMaster64 uses the D64 format to read or create disk images. The structure of a D64 file can be found [here](http://unusedino.de/ec64/technical/formats/d64.html). The D64 format is supported by all C64 emulators as well as by [SD2IEC](https://github.com/wagiminator/C64-Collection/tree/master/C64_SD2IEC_LP) and Pi1541.

## Compiling and Uploading the Firmware
### Preparing the CH55x Bootloader
#### Installing Drivers for the CH55x Bootloader
On Linux you do not need to install a driver. However, by default Linux will not expose enough permission to upload your code with the USB bootloader. In order to fix this, open a terminal and run the following commands:

```
echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="4348", ATTR{idProduct}=="55e0", MODE="666"' | sudo tee /etc/udev/rules.d/99-ch55x.rules
sudo service udev restart
```

For Windows, you need the [CH372 driver](http://www.wch-ic.com/downloads/CH372DRV_EXE.html). Alternatively, you can also use the [Zadig Tool](https://zadig.akeo.ie/) to install the correct driver. Here, click "Options" and "List All Devices" to select the USB module, and then install the libusb-win32 driver. To do this, the board must be connected and the CH55x must be in bootloader mode.

#### Entering CH55x Bootloader Mode
A brand new chip starts automatically in bootloader mode as soon as it is connected to the PC via USB. Once firmware has been uploaded, the bootloader must be started manually for new uploads. To do this, the board must first be disconnected from the USB port and all voltage sources. Now press the BOOT button and keep it pressed while reconnecting the board to the USB port of your PC. The chip now starts again in bootloader mode, the BOOT button can be released and new firmware can be uploaded within the next couple of seconds.

If a DiskMaster64 firmware is already installed, the corresponding Python scripts can also put the microcontroller into boot mode on the software side. In this case, it is no longer necessary to press the BOOT button.

### Preparing the Device for Uploading
- Disconnect the device from the floppy disk drive and the USB port. 
- Press the BOOT button on the device and keep it pressed while connecting to the USB port. Pressing the BOOT button is not required if DiskMaster64 firmware is already installed and one of the Python scripts is used for uploading.
- Use one of the following methods:

### If using the graphical front end (recommended)
- Execute the respective Python script on your PC: `diskmaster-gui.py`.
- Click on <kbd>Flash firmware</kbd>.

### If using the command line tool
- Execute the respective Python script on your PC: `python3 flash-firmware.py`.

### If using the Arduino IDE
#### Installing the Arduino IDE and CH55xduino
Install the [Arduino IDE](https://www.arduino.cc/en/software) if you haven't already. Install the [CH55xduino](https://github.com/DeqingSun/ch55xduino) package by following the instructions on the website.

#### Compiling and Uploading Firmware
- Copy the .ino and .c files as well as the /src folder together into one folder and name it diskmaster64. 
- Open the .ino file in the Arduino IDE.
- Go to **Tools -> Board -> CH55x Boards** and select **CH552 Board**.
- Go to **Tools** and choose the following board options:
  - **Clock Source:**   16 MHz (internal)
  - **Upload Method:**  USB
  - **USB Settings:**   USER CODE /w 0B USB RAM
- Connect the board and make sure the CH55x is in bootloader mode. 
- Click **Upload**.

### If using the makefile (Linux)
#### Installing SDCC Toolchain for CH55x
Install the [SDCC Compiler](https://sdcc.sourceforge.net/). In order for the programming tool to work, Python3 must be installed on your system. To do this, follow these [instructions](https://www.pythontutorial.net/getting-started/install-python/). In addition [pyusb](https://github.com/pyusb/pyusb) must be installed. On Linux (Debian-based), all of this can be done with the following commands:

```
sudo apt install build-essential sdcc python3 python3-pip
sudo pip install pyusb
```

#### Compiling and Uploading Firmware
- Open a terminal.
- Navigate to the folder with the makefile. 
- Connect the board and make sure the CH55x is in bootloader mode. 
- Run ```make flash``` to compile and upload the firmware. 
- If you don't want to compile the firmware yourself, you can also upload the precompiled binary. To do this, just run ```python3 ./tools/chprog.py diskmaster64.bin```.

## Installing Python and Drivers
Python3 needs to be installed on your PC in order to use the software. Most Linux distributions already include this. Windows users can follow these [instructions](https://www.pythontutorial.net/getting-started/install-python/). In addition PySerial and Tkinter (8.6 or newer) must be installed. However, these are already included in most Python installations.

Windows users may also need to install a CDC driver using the [Zadig Tool](https://zadig.akeo.ie/). Here, click "Options" and "List All Devices" to select the "CDC Serial", and then install the CDC driver.

# Operating Instructions
## Preparation
- Connect the adapter to your Commodore 1541(II) floppy disk drive via the IEC cable.
- Connect the adapter to a USB port of your PC.
- Switch on your floppy disk drive.
- Use the provided Python scripts in the software/pc/ folder on your PC.

## Graphical Front End
- Execute the respective Python script on your PC: `diskmaster-gui.py`.
- The rest should be self explanatory.

![DiskMaster64_gui.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_DiskMaster64/documentation/DiskMaster64_gui.jpg)

## Command Line Interface
The following command line tools are available:

### Reading status of connected IEC devices
```
python3 disk-detect.py
```

### Reading directory of an inserted floppy disk
```
python3 disk-dir.py [-h] [-d {8,9,10,11}]

optional arguments:
-h, --help                  show help message and exit
-d, --device                device number of disk drive (8-11, default=8)
```

### Formatting an inserted floppy disk
```
python3 disk-format.py [-h] [-x] [-n] [-c] [-v] [-d {8,9,10,11}] [-t TITLE] [-i IDENT]

optional arguments:
-h, --help                  show help message and exit
-x, --extend                format 40 tracks
-n, --nobump                do not bump head
-c, --clear                 clear (demagnetize) disk (recommended for new disks)
-v, --verify                verify each track after it is formatted
-d, --device                device number of disk drive (8-11, default=8)
-t TITLE, --title TITLE     disk title (max 16 characters, default=commodore)
-i IDENT, --ident IDENT     disk ident (2 characters, default=64)

Example: python disk-format.py -t games -i a7
```

### Reading from floppy disk to a D64 image file
```
python3 disk-read.py [-h] [-x] [-b] [-d {8,9,10,11}] [-i INTER] [-f FILE]

optional arguments:
-h, --help                    show help message and exit
-x, --extend                  read disk with 40 tracks
-b, --bamonly                 only read blocks with BAM entry (recommended)
-d, --device                  device number of disk drive (8-11, default=8)
-i INTER, --interleave INTER  sector interleave (default=4)
-f FILE, --file FILE          output file (default=output.d64)

Example: python disk-read.py -b -f game.d64
```

### Writing a D64 image file to floppy disk
```
python3 disk-write.py [-h] [-b] [-d {8,9,10,11}] [-i INTER] -f FILE

optional arguments:
-h, --help                    show help message and exit
-b, --bamonly                 only write blocks with BAM entry (recommended)
-d, --device                  device number of disk drive (8-11, default=8)
-i INTER, --interleave INTER  sector interleave (default=4)
-f FILE, --file FILE          input file (*.d64)

Example: python disk-write.py -b -f game.d64
```

### Comparing D64 image file with floppy disk content (verify)
```
python3 disk-verify.py [-h] [-b] [-d {8,9,10,11}] [-i INTER] [-e ERRORS] -f FILE

optional arguments:
-h, --help                    show help message and exit
-b, --bamonly                 only verify blocks with BAM entry (recommended)
-d, --device                  device number of disk drive (8-11, default=8)
-i INTER, --interleave INTER  sector interleave (default=4)
-e ERRORS, --errors ERRORS    tolerated errors until abort (default=0)
-f FILE, --file FILE          d64 file to compare the disk with

Example: python disk-verify.py -b -f game.d64
```

### Reading PRG files from floppy disk
```
python3 disk-load.py [-h] [-d {8,9,10,11}]

optional arguments:
-h, --help            show help message and exit
-d, --device          device number of disk drive (8-11, default=8)
```

### Flashing firmware to DiskMaster64 adapter
```
python3 flash-firmware.py
```

## Working with D64 Files
There are numerous applications to create and modify D64 image files and add or delete PRG files yourself. The whole thing is very quick and easy, for example with [WebDrive](https://cbm8bit.com/webdrive/) on [cbm8bit.com](https://cbm8bit.com/). As a web application, it is platform-independent and requires no installation. In addition, you can even visualize the image of the diskette - very cool!

![DumpMaster64_output.png](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_DumpMaster64/documentation/DumpMaster64_output.png)

If you are looking for a command line tool for working with D64 files, [cc1541](https://bitbucket.org/PTV_Claus/cc1541) is a good choice.

# References, Links and Notes
1. [SD2IEC Project](https://github.com/wagiminator/C64-Collection/tree/master/C64_SD2IEC_LP)
2. [XU1541 Project](https://github.com/wagiminator/C64-Collection/tree/master/C64_xu1541)
3. [DiskBuddy64 Project](https://github.com/wagiminator/C64-Collection/tree/master/C64_DiskBuddy64)
4. [Michael Steil: Commodore Peripheral Bus](https://www.pagetable.com/?p=1018)
5. [Michael Steil: A 256 Byte Autostart Fast Loader](https://www.pagetable.com/?p=568)
6. [Michael Steil: Ultimate Commodore 1541 Drive Talk](https://youtu.be/_1jXExwse08)
7. [Jim Butterfield: How The VIC/64 Serial Bus Works](http://www.zimmers.net/anonftp/pub/cbm/programming/serial-bus.txt)
8. [J. Derogee: IEC disected](http://www.zimmers.net/anonftp/pub/cbm/programming/serial-bus.pdf)
9. [Immers/Neufeld/Moser: D64 File Format](http://unusedino.de/ec64/technical/formats/d64.html)
10. [OpenCBM Software](https://github.com/OpenCBM/OpenCBM)
11. [cc1541 Software](https://bitbucket.org/PTV_Claus/cc1541)
12. [cbm8bit WebDrive](https://cbm8bit.com/webdrive/)
13. [CH551/552 Datasheet](http://www.wch-ic.com/downloads/CH552DS1_PDF.html)
14. [SDCC Compiler](https://sdcc.sourceforge.net/)
15. [EasyEDA Design Files](https://oshwlab.com/wagiminator)

![DiskMaster64_pic3.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_DiskMaster64/documentation/DiskMaster64_pic3.jpg)
![DiskMaster64_pic4.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_DiskMaster64/documentation/DiskMaster64_pic4.jpg)
![DiskMaster64_pic5.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_DiskMaster64/documentation/DiskMaster64_pic5.jpg)

# License
![license.png](https://i.creativecommons.org/l/by-sa/3.0/88x31.png)

This work is licensed under Creative Commons Attribution-ShareAlike 3.0 Unported License. 
(http://creativecommons.org/licenses/by-sa/3.0/)
