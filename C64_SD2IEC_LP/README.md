# SD2IEC - C64 SD Card Adapter
SD2IEC is a C64 SD card adapter and floppy disk drive emulator based on the design by [Lars Pontoppidan](https://larsee.com/blog/2007/02/the-mmc2iec-device/) with cassette port plug. Copy C64 games and programs to SD card on your PC, put the SD card in your SD2IEC and load games and programs on your C64!

![C64_SD2IEC_LP_pic1.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_SD2IEC_LP/documentation/C64_SD2IEC_LP_pic1.jpg)

The SD2IEC is a mass storage device using an SD/MMC card and interfacing with the IEC bus. It is based on the ATmega644P microcontroller. The most prominent use of SD2IEC is replacement of a Commodore 1541 disk drive for a C64, C128, C16 or VIC-20. It supports reading and writing the following file types: D64, D71, D81, PRG and P00 and works with many fast loaders, including [EPYX Fastload](https://github.com/wagiminator/C64-Collection/tree/master/C64_Cartridge_FreeLoad). However, SD2IEC is not a complete disk drive emulator. Not every game will work from SD2IEC!

# Installing the Firmware
An AVR programmer is required to flash the firmware for the first time. A cheap and reliable programmer is e.g. [USBasp](https://aliexpress.com/wholesale?SearchText=usbasp). The SD2IEC must not be connected to the C64 during flashing!

- Remove the SD card. 
- Connect the programmer to the PC via USB and to the ICSP header on the board via an ICSP cable.
- Make sure you have installed [avrdude](https://learn.adafruit.com/usbtinyisp/avrdude).
- Open a terminal.
- Navigate to the software folder with the hex-file.
- Execute the following commands one after the other (if necessary replace "usbasp" with the programmer you use):
  ```
  avrdude -c usbasp -p m644p -U flash:w:sd2iec-1.0.0-larsp-m644p.bin:a
  avrdude -c usbasp -p m644p -U hfuse:w:0x91:m -U lfuse:w:0xef:m -U efuse:w:0xfd:m
  ```

Firmware updates can be downloaded from https://www.sd2iec.de/nightlies/ (use firmware ...larsp-m644p.bin). Copy the BIN file to the root of the SD card. It is automatically flashed the next time the device is started up.

# Operating Instructions
- Format an SD card (max 32GB, 1GB or 2GB cards work best) to FAT32 file system on your PC.
- Copy files from /software/sdcard_root to the root of the SD card.
- [Download](https://www.c64games.de/) and copy C64 programs/games to root or organize them in folders.
- Put the SD card to SD2IEC.
- Plug the SD2IEC floppy emulator to the cassette port of your C64.
- Connect the IEC cable to the serial port of your C64.
- Turn on your C64.
- Type: LOAD "FB64",8 <kbd>RETURN</kbd>
- After the program loaded type: RUN <kbd>RETURN</kbd>
- This should load and run filebrowser. To navigate the menu either use cursor keys or Joystick in port2.  For single D64 images, just open the D64 by pressing <kbd>RETURN</kbd> or <kbd>FIRE</kbd> and then select the first PRG file to LOAD.

# DIP switches
You can change the address of your SD2IEC with the DIP switches:

|DIP1|DIP2|Address|
|-|-|-|
|OFF|OFF|8|
|ON|OFF|9|
|OFF|ON|10|
|ON|ON|11|

# References, Links and Notes
1. [Original Project](https://larsee.com/blog/2007/02/the-mmc2iec-device/)
2. [Firmware Download](https://www.sd2iec.de/nightlies/)
3. [Inofficial Manual](https://www.thegeekpub.com/9473/sd2iec-manual-use-sd2iec-c64/)
4. [Another Manual](http://www.c64os.com/post/sd2iecdocumentation)
5. [And another one](https://www.thefuturewas8bit.com/index.php/sd2iec-info)
5. [C64 Games](https://www.c64games.de/)
6. [C64 Demos](https://csdb.dk/)

![C64_SD2IEC_LP_pic2.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_SD2IEC_LP/documentation/C64_SD2IEC_LP_pic2.jpg)
