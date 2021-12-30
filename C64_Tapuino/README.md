# Tapuino - Commodore Datasette Emulator
Tapuino is a Commodore Datasette emulator for C16, C64, C128 and VIC-20 based on the design and firmware by [Sweetlilmre](https://github.com/sweetlilmre/tapuino). Put your TAP files on an SD-Card and plug the device in your good old Commodore.

![C64_Tapuino_pic1.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_Tapuino/documentation/C64_Tapuino_pic1.jpg)

# Installing the Firmware
An AVR programmer is required to flash the firmware. A cheap and reliable programmer is e.g. [USBasp](https://aliexpress.com/wholesale?SearchText=usbasp). 

- Connect the programmer to the PC via USB and to the ICSP header on the board via an ICSP cable.
- Make sure you have installed [avrdude](https://learn.adafruit.com/usbtinyisp/avrdude).
- Open a terminal.
- Navigate to the software folder with the hex-file.
- Execute the following command (if necessary replace "usbasp" with the programmer you use):
  ```
  avrdude -c usbasp -p m328p -U flash:w:tapuino.hex -U lfuse:w:0xff:m -U hfuse:w:0xdb:m -U efuse:w:0xfd:m
  ```

# Operating Instructions
## Loading a TAP File
Copy your TAP files to the SD card and insert the card into the SD card slot. Switch on your C64, enter the command "LOAD" and confirm with "Enter". Now select the desired file to be loaded on the Tapuino. Use the four buttons to do this and proceed as follows:
- Select the option "Load"
- Navigate to the desired file
- Confirm the choice

The game/program is now loaded to the C64 and the progress bar on the OLED display shows the loading status in percent.

## Saving a File
Writing data onto the SD card is just as easy as reading from the card. All you have to do is select the "Record" option in the menu. Here are the steps:

On the C64: Load the data to be written to the card or write a little Basic program just for test purposes. Then initiate the saving process with the command "SAVE".

On the Tapuino:
- Select the menu item "Record"
- Select the recording mode. With "Manual" the file name must be entered manually, with "Automatic" a predefined name is generated automatically. If you want to choose the name yourself (manually) you have to type in the name using the buttons, which is very cumbersome.
  |Button|Function|
  |:-|:-|
  |NEXT|cursor left|
  |PREV|cursor to the right|
  |BACK/SELECT|change letters/characters|
  |Press and hold NEXT|confirm|
- When "Ready ..." is shown on the display, you only have to press button 1 once more, and the storage process begins.

The saved data is saved on the SD card in the directory "Recordings". These can also be loaded from there.

## Converting Programs to TAP Format
Not every program can be found in TAP format, but there is a way to convert images with a different format to TAP. The small Windows program [WAV-PRG](http://wav-prg.sourceforge.net/wavprg.html) enables PRG, P00- and T64 files to be converted into TAP format. After opening the program, choose the second option "Convert a PRG, P00 or T64 to a sound or TAP or WAV" and click "OK". Now select the machine for which the file is to be converted from the drop-down menu, e.g. C64 PAL and click "OK". Now select the file to be converted. That's it, the TAP file is now ready for use with the Tapuino.

## Dumping Tapes from your old Datasette
If you still have old tapes and a Commodore Datasette, you can use them to create TAP images. To do this, check out the project [TapeBuddy64](https://github.com/wagiminator/C64-Collection/tree/master/C64_TapeBuddy64).

# References, Links and Notes
1. [Source Code](https://github.com/sweetlilmre/tapuino)
2. [Tapuino Tutorial (german)](https://www.mingos-commodorepage.com/tutorials/c64tapuino.php?title=Der%20Tapuino%20im%20Selbstbau)
3. [TapeBuddy64](https://github.com/wagiminator/C64-Collection/tree/master/C64_TapeBuddy64)
4. [C64 Games](https://www.c64games.de/)

![C64_Tapuino_pic3.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_Tapuino/documentation/C64_Tapuino_pic3.jpg)
![C64_Tapuino_pic2.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_Tapuino/documentation/C64_Tapuino_pic2.jpg)
![C64_Tapuino_pic4.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_Tapuino/documentation/C64_Tapuino_pic4.jpg)
![C64_Tapuino_pic5.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_Tapuino/documentation/C64_Tapuino_pic5.jpg)
