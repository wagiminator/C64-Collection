# DumpMaster64 - Connect your Commodore Datasette or 1541 Floppy Disk Drive to your PC
The DumpMaster64 adapter bridges the gap between your modern PC and your ancient mass storage devices for the Commodore C64. It can interface Commodore 1541(II) floppy disk drives as well as Commodore C2N/1530 Datasettes. It is a combination of the [DiskBuddy64](https://github.com/wagiminator/C64-Collection/tree/master/C64_DiskBuddy64) and the [TapeBuddy64](https://github.com/wagiminator/C64-Collection/tree/master/C64_TapeBuddy64). The DumpMaster64 is controlled via a command line interface or a graphical front end written in Python.

The DumpMaster64 is a proprietary interface. It does not turn the floppy disk drive or the Datasette into a standard USB mass storage device. Instead, it offers the ability to send low-level commands back and forth over USB. The provided Python scripts were developed to support this interface and can directly access the floppy disk drive and the contents of an inserted floppy disk or read from and write on tapes. Thanks to the integrated fast loader, the adapter reads or writes floppy disks at twelve times the normal speed, dumping an entire disk in about 32 seconds. DumpMaster64 cannot read or write copy-protected disks.

The integrated USB to serial converter can also be used as a SerialUPDI programmer for the on-board ATtiny microcontroller, so that no additional hardware is required to flash the firmware.

![DumpMaster64_pic1.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_DumpMaster64/documentation/DumpMaster64_pic1.jpg)

# Hardware
The schematic is shown below:

![DumpMaster64_wiring.png](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_DumpMaster64/documentation/DumpMaster64_wiring.png)

## Microcontroller
One of the (not quite so) new 14-pin [tinyAVRs](https://ww1.microchip.com/downloads/en/DeviceDoc/ATtiny804-06-07-1604-06-07-DataSheet-DS40002312A.pdf) has been chosen for this project. It has enough pins, hardware UART and a timer/counter B (TCB), which is specially designed for CPU-independent frequency and pulse length measurements. Any of these MCUs can be used: ATtiny804, ATtiny814, ATtiny1604, or ATtiny1614. Don't buy them in China, they are much cheaper e.g. at [Digikey](https://www.digikey.de/de/products/detail/microchip-technology/ATTINY804-SSN/10270320) (€0.68).

## USB to Serial Converter
The [CH340N](https://datasheet.lcsc.com/lcsc/2101130932_WCH-Jiangsu-Qin-Heng-CH340N_C506813.pdf) (or CH330N) is now in use everywhere with me. At around €0.45 (if you buy [10 pieces](https://lcsc.com/product-detail/USB-ICs_WCH-Jiangsu-Qin-Heng-CH340N_C506813.html)) it is very cheap, fast, small, uncomplicated, reliable and hardly requires any external components (not even a crystal). It's probably the best choice for this small project too. And it is also used as a SerialUPDI programmer for the ATtiny.

## Boost Converter
In order to provide the approximately 6V for the motor of the Datasette, a boost converter based on the inexpensive [MT3608](https://datasheet.lcsc.com/lcsc/1811151539_XI-AN-Aerosemi-Tech-MT3608_C84817.pdf) is installed on the board. Controlling the motor with 5V would certainly also be possible, but the reduced speed would then have to be compensated by software later. It is difficult to estimate what this would mean for the accuracy of the sampled data. The ATtiny can switch the motor on and off via a MOSFET.

## IEC Connector
The floppy disk drive is connected to the adapter via the [6-pin DIN jack](https://aliexpress.com/item/1005001265863686.html). Alternatively, a serial cable can be soldered directly to the corresponding pads.

## Cassette Port
The cassette port is a 12-pin edge connector as part of the printed circuit board (PCB) with a spacing of 3.96mm, where 6 pins are on each sides (top and down) of the PCB. The opponent contacts are connected with each other, A with 1, B with 2, and so on. The connector of the datasette is pushed directly onto this port. A notch between B/2 and C/3 prevents accidental polarity reversal.

![DumpMaster64_hardware.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_DumpMaster64/documentation/DumpMaster64_hardware.jpg)

# Software
## Theory of Operation
For a more detailed insight into how it works, refer to the [TapeBuddy64](https://github.com/wagiminator/C64-Collection/tree/master/C64_TapeBuddy64) and [DiskBuddy64](https://github.com/wagiminator/C64-Collection/tree/master/C64_DiskBuddy64) projects, since the DumpMaster64, to put it simply, is just a combination of both devices on a single board and in a single microcontroller.

## Compiling and Uploading the Firmware
- Set the serial mode switch on the device to "UPDI". 
- Connect the device to a USB port of your PC.
- Use one of the following methods:

### If using the graphical front end (recommended)
- Execute the respective Python script on your PC: `python dumpmaster-gui.py`.
- Click on <kbd>Flash firmware</kbd>.

### If using the command line tool
- Execute the respective Python script on your PC: `python flash-firmware.py`.

### If using the Arduino IDE
- Open your Arduino IDE.
- Make sure you have installed [megaTinyCore](https://github.com/SpenceKonde/megaTinyCore).
- Go to **Tools -> Board -> megaTinyCore** and select **ATtiny1614/1604/814/804/414/404/214/204**.
- Go to **Tools** and choose the following board options:
  - **Chip:**           Select the chip you have installed on your device
  - **Clock:**          16 MHz internal
  - **Programmer:**     SerialUPDI
  - Leave the rest at the default settings.
- Go to **Tools -> Burn Bootloader** to burn the fuses.
- Open the DumpMaster64 sketch in the software/avr folder and click **Upload**.

### If using the makefile (Linux/Mac)
- Make sure you have installed the latest [avr-gcc toolchain](http://maxembedded.com/2015/06/setting-up-avr-gcc-toolchain-on-linux-and-mac-os-x/).
- Open a terminal.
- Navigate to the folder with the makefile and the Arduino sketch.
- Run `DEVICE=attiny804 make install` to compile, burn the fuses and upload the firmware (change DEVICE accordingly).

## Installing Python and Drivers
Python needs to be installed on your PC in order to use the software. Most Linux distributions already include this. Windows users can follow these [instructions](https://www.pythontutorial.net/getting-started/install-python/). In addition PySerial and Tkinter (8.6 or newer) must be installed. However, these are already included in most Python installations.

Windows users may also need to install a [driver](http://www.wch.cn/download/CH341SER_ZIP.html) for the CH340N USB to serial adapter. This is not necessary for Linux or Mac users.

# Operating Instructions
## Preparation
- Set the serial mode switch on your DumpMaster64 adapter to "UART".
- Connect the adapter to your Datasette and/or your floppy disk drive(s).
- Connect the adapter to a USB port of your PC.
- Switch on your floppy disk drive(s) if connected.
- Use the provided Python scripts in the software/pc/ folder on your PC.

## Graphical User Interface
- Execute the respective Python script on your PC: `python dumpmaster-gui.py`.
- The rest should be self explanatory.

![DumpMaster64_gui.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_DumpMaster64/documentation/DumpMaster64_gui.jpg)

## Command Line Interface
The following command line tools are available:

### Reading status of connected IEC devices
```
python disk-detect.py
```

### Reading directory of an inserted floppy disk
```
python disk-dir.py [-h] [-d {8,9,10,11}]

optional arguments:
-h, --help                  show help message and exit
-d, --device                device number of disk drive (8-11, default=8)
```

### Formatting an inserted floppy disk
```
python disk-format.py [-h] [-x] [-n] [-c] [-v] [-d {8,9,10,11}] [-t TITLE] [-i IDENT]

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
python disk-read.py [-h] [-x] [-b] [-d {8,9,10,11}] [-i INTER] [-f FILE]

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
python disk-write.py [-h] [-b] [-d {8,9,10,11}] [-i INTER] -f FILE

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
python disk-verify.py [-h] [-b] [-d {8,9,10,11}] [-i INTER] [-e ERRORS] -f FILE

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
python disk-load.py [-h] [-d {8,9,10,11}]

optional arguments:
-h, --help            show help message and exit
-d, --device          device number of disk drive (8-11, default=8)
```

### Reading from tape to TAP file
```
python tape-read.py FILE

Example: python tape-read.py outputfile.tap
```

### Writing TAP file to tape
```
python tape-write.py FILE

Example: python tape-write.py inputfile.tap
```

### Flashing firmware to DiskBuddy64 adapter
```
python flash-firmware.py
```

## Status LEDs
|LED|Description|
|:-|:-|
|PWR|Steady: device is powered via USB|
|BUSY|Steady: device is accessing a disk or cassette<br>Flashing: device waits for the prompted key on the Datasette being pressed|

## Working with D64 Files
There are numerous applications to create and modify D64 image files and add or delete PRG files yourself. The whole thing is very quick and easy, for example with [WebDrive](https://cbm8bit.com/webdrive/) on [cbm8bit.com](https://cbm8bit.com/). As a web application, it is platform-independent and requires no installation. In addition, you can even visualize the image of the diskette - very cool!

![DumpMaster64_output.png](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_DumpMaster64/documentation/DumpMaster64_output.png)

If you are looking for a command line tool for working with D64 files, [cc1541](https://bitbucket.org/PTV_Claus/cc1541) is a good choice.

## Checking, Cleaning and Converting TAP Files
The TAP files generated by DumpMaster64 can be used directly in your favorite emulator or with the [Tapuino](https://github.com/wagiminator/C64-Collection/tree/master/C64_Tapuino), but perhaps you want to check it beforehand, optimize it or convert it into another format. This works perfectly with the command line tool [TAPClean](https://sourceforge.net/projects/tapclean/). To check the quality of the generated TAP image, use the following command:
`tapclean -t outputfile.tap`
Remember, a TAP file is almost never 100% clean, as pauses in particular are not always correctly recognized. You can optimize the quality of the file with the following command:
`tapclean -o outputfile.tap`
Slight deviations in the pulse lengths are compensated for and everything is adapted to the exact timing specifications. TAPClean not only knows the required pulse lengths of the standard Commodore coding, but also those of all common fast loaders.
If you want to convert the TAP file into a WAV audio file, then use the following command:
`tapclean -wav outputfile.tap`
There is also a graphical [front end](https://www.luigidifraia.com/software/) for TAPClean.

## Datasette Head Alignment
If you have problems loading programs from tape, the azimuth of the tape head may be out of position. Before setting the azimuth, however, all other causes of error should be ruled out. Try other cassettes first, as the tapes will age over time and the signals will weaken. Also clean the connector contacts with contact cleaner and the read head with isopropyl alcohol. If it still doesn't work, it's time for the screwdriver.

To make the azimuth setting, there is a tiny screw hole above the rewind button through which a small jeweler's screwdriver can fit. When the tape drive is playing, the hole will align with the head adjustment screw. Don't put too much pressure on and avoid over-tightening or over-loosening the screw! Follow these steps:
- Insert a cassette into the Datasette that you know is working well.
- Execute `python dumpmaster-gui.py`
- Select "Read from tape to TAP file", then select the output file. You can delete it later.
- Press <kbd>PLAY</kbd> on the Datasette when promted.
- As soon as the pulse lines are displayed, take a thin screwdriver and slowly adjust the azimuth until the displayed lines are as narrow as possible.

![TapeBuddy64_align2.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_TapeBuddy64/documentation/TapeBuddy64_align2.jpg)
![TapeBuddy64_align1.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_TapeBuddy64/documentation/TapeBuddy64_align1.jpg)

# References, Links and Notes
1. [DiskBuddy64 Project](https://github.com/wagiminator/C64-Collection/tree/master/C64_DiskBuddy64)
2. [TapeBuddy64 Project](https://github.com/wagiminator/C64-Collection/tree/master/C64_TapeBuddy64)
3. [SD2IEC Project](https://github.com/wagiminator/C64-Collection/tree/master/C64_SD2IEC_LP)
4. [XU1541 Project](https://github.com/wagiminator/C64-Collection/tree/master/C64_xu1541)
5. [Tapuino Project](https://github.com/wagiminator/C64-Collection/tree/master/C64_Tapuino)
6. [Immers/Neufeld/Moser: D64 File Format](http://unusedino.de/ec64/technical/formats/d64.html)
7. [Schepers/Sundell/Brenner: TAP File Format](https://ist.uwaterloo.ca/~schepers/formats/TAP.TXT)
8. [Michael Steil: Commodore Peripheral Bus](https://www.pagetable.com/?p=1018)
9. [Michael Steil: A 256 Byte Autostart Fast Loader](https://www.pagetable.com/?p=568)
10. [Michael Steil: Ultimate Commodore 1541 Drive Talk](https://youtu.be/_1jXExwse08)
11. [Jim Butterfield: How The VIC/64 Serial Bus Works](http://www.zimmers.net/anonftp/pub/cbm/programming/serial-bus.txt)
12. [J. Derogee: IEC disected](http://www.zimmers.net/anonftp/pub/cbm/programming/serial-bus.pdf)
13. [Harrie De Ceukelaire: How TurboTape Works](https://www.atarimagazines.com/compute/issue57/turbotape.html)
14. [Luigi Di Fraia: Analyzing C64 Tape Loaders](https://github.com/binaryfields/zinc64/blob/master/doc/Analyzing%20C64%20tape%20loaders.txt)
15. [Michael Steil: Archiving C64 Tapes Correctly](https://www.pagetable.com/?p=1002)
16. [OpenCBM Software](https://github.com/OpenCBM/OpenCBM)
17. [cc1541 Software](https://bitbucket.org/PTV_Claus/cc1541)
18. [cbm8bit WebDrive](https://cbm8bit.com/webdrive/)
19. [TAPClean Software](https://sourceforge.net/projects/tapclean/)
20. [TAPClean Front End](https://www.luigidifraia.com/software/)
21. [CBM: 1540/1541 Service Manual](http://www.zimmers.net/anonftp/pub/cbm/schematics/drives/new/1541/1540-1541_Disk_Drive_Service_Manual_314002-01_(1985_Nov).pdf)
22. [CBM: Datasette Service Manual](https://www.vic-20.it/wp-content/uploads/C2N-1530-1531_service_manual.pdf)
23. [Microchip: ATtiny Datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/ATtiny804-06-07-1604-06-07-DataSheet-DS40002312A.pdf)
24. [WCH: CH340N Datasheet](https://datasheet.lcsc.com/lcsc/2101130932_WCH-Jiangsu-Qin-Heng-CH340N_C506813.pdf)
25. [AeroSemi: MT3608 Datasheet](https://datasheet.lcsc.com/lcsc/1811151539_XI-AN-Aerosemi-Tech-MT3608_C84817.pdf)

![DumpMaster64_pic2.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_DumpMaster64/documentation/DumpMaster64_pic2.jpg)

# License
![license.png](https://i.creativecommons.org/l/by-sa/3.0/88x31.png)

This work is licensed under Creative Commons Attribution-ShareAlike 3.0 Unported License. 
(http://creativecommons.org/licenses/by-sa/3.0/)
