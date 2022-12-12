# TapeDump64 - Dump your Commodore Datasette Tapes
TapeDump64 is a simple and inexpensive adapter that can interface a Commodore Datasette to your PC via USB in order to dump your old software saved on tapes as TAP files. It is inspired by [TrueTape64](https://github.com/francescovannini/truetape64), a similar project by Francesco Vannini. The integrated USB to serial converter can also be used as a SerialUPDI programmer for the on-board ATtiny microcontroller, so that no additional hardware is required to flash the firmware. For an advanced version that can also write on tapes, take a look at [TapeBuddy64](https://github.com/wagiminator/C64-Collection/tree/master/C64_TapeBuddy64).

![TapeDump64_pic1.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_TapeDump64/documentation/TapeDump64_pic1.jpg)

# Hardware
The schematic is shown below:

![TapeDump64_wiring.png](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_TapeDump64/documentation/TapeDump64_wiring.png)

## Microcontroller
I finally wanted to use one of the (not quite so) new 8-pin [tinyAVRs](https://ww1.microchip.com/downloads/en/DeviceDoc/ATtiny202-204-402-404-406-DataSheet-DS40002318A.pdf) for a project. Here it fits really well. It has enough pins, hardware UART and a timer/counter B (TCB), which is specially designed for CPU-independent frequency and pulse length measurements. Since the firmware requires less than 800 bytes of flash, any of these MCUs can be used: ATtiny202, ATtiny212, ATtiny402 or ATtiny412. Don't buy them in China, they are much cheaper e.g. at [Digikey](https://www.digikey.de/de/products/detail/microchip-technology/ATTINY202-SSN/9947534) (€0.45).

## USB to Serial Converter
The [CH340N](https://datasheet.lcsc.com/lcsc/2101130932_WCH-Jiangsu-Qin-Heng-CH340N_C506813.pdf) (or CH330N) is now in use everywhere with me. At around €0.45 (if you buy [10 pieces](https://lcsc.com/product-detail/USB-ICs_WCH-Jiangsu-Qin-Heng-CH340N_C506813.html)) it is very cheap, fast, small, uncomplicated, reliable and hardly requires any external components (not even a crystal). It's probably the best choice for this small project too. And it is also used as a SerialUPDI programmer for the ATtiny.

## Boost Converter
In order to provide the approximately 6.1V for the motor of the Datasette, a boost converter based on the inexpensive [MT3608](https://datasheet.lcsc.com/lcsc/1811151539_XI-AN-Aerosemi-Tech-MT3608_C84817.pdf) is installed on the board. Controlling the motor with 5V would certainly also be possible, but the reduced speed would then have to be compensated by software later. It is difficult to estimate what this would mean for the accuracy of the sampled data.

## Cassette Port
The cassette port is a 12-pin edge connector as part of the printed circuit board (PCB) with a spacing of 3.96mm, where 6 pins are on each sides (top and down) of the PCB. The opponent contacts are connected with each other, A with 1, B with 2, and so on. The connector of the datasette is pushed directly onto this port. A notch between B/2 and C/3 prevents accidental polarity reversal.

|Pin|Signal|Description|
|:-|:-|:-|
|A / 1|GND|Ground|
|B / 2|+5V|5 Volt DC|
|C / 3|MOTOR|Motor Control, approx. 6.1 Volt power supply of the motor|
|D / 4|READ|Data Input, read data from Datasette|
|E / 5|WRITE|Data Output, write data to Datasette (not used here)|
|F / 6|SENSE|Detection, if one of the keys PLAY, RECORD, F.FWD or REW is pressed|

![TapeDump64_hardware.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_TapeDump64/documentation/TapeDump64_hardware.jpg)

# Software
## Working Principle
The data is stored on the tape as a frequency modulated waveform. This signal is read from the Datasette, amplified and converted into a TTL-level square wave with the same pulse lengths and then output to the READ pin of the cassette port. TapeDump64 reads this square wave and measures the pulse durations between the falling edges of the signal. The associated TAP data bytes are calculated from the pulse lengths, which are then sent to the PC via USB and written there into the TAP file.

![TapeDump64_principle.png](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_TapeDump64/documentation/TapeDump64_principle.png)

## Interpretation of the Pulse Lengths
The actual data is coded using a sequence of pulses of different lengths. The standard Commodore system uses patterns of three different pulse lengths: short (390µs), medium (536µs) and long (698µs). Each byte of data is preceded by a marker consisting of a long pulse followed by a medium one. A "0" bit is represented by a short pulse followed by a medium one, while a "1" bit is a medium pulse followed by a short one. Each byte on the tape ends with a parity bit.

However, fast loaders are often used, each of which uses different pulse lengths and patterns, which complicates matters overall. Fortunately, TapeDump64 doesn't have to decode the data. This is because only the raw data is saved in the TAP file in the form of the individual pulse lengths. The actual interpretation of these pulses is done later by the C64 or a corresponding emulator.

## Timing Accuracy
The accuracy of the measurement of the pulse lengths depends on various factors. For example, not all motors in the Datasettes run at exactly the same speed. In addition, the C64 measures the pulse length via the timer of the CIA as the number of passed system clock cycles, but the PAL versions of the C64 have a different clock frequency (985248 Hz) than the NTSC versions (1022730 Hz). Badly aligned read heads of the Datasette as well as old tapes with weakened signals contribute to the rest.

That's why the difference between the individual pulse lengths used for the encoding of the data is large enough to be able to reliably differentiate between them even under the circumstances mentioned. In fact, it is not the measured pulse length itself that is relevant, but whether it is above or below a defined threshold value. Furthermore, turbo loaders often use a mechanism to synchronize with the data stream from the datasette.

## TAP File Format
The TAP file format attempts to duplicate the data stored on a C64 cassette tape, bit for bit. It was designed by Per Håkan Sundell (author of the [CCS64](http://www.ccs64.com/) C64 emulator) in 1997. Since it is simply a representation of the raw serial data from a tape, it should handle any custom tape loaders that exist. The layout is fairly simple, with a small 20-byte header followed by the file data:

![TapeDump64_format.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_TapeDump64/documentation/TapeDump64_format.jpg)

|Bytes (hex)|Description|
|:-|:-|
|0000-000B|File signature "C64-TAPE-RAW"|
|000C|TAP version (0 - original, 1 - updated, 2 - halfwave extension)|
|000D|Computer platform (0 - C64, 1 - VIC-20, 2 - C16, Plus/4, 3 - PET)|
|000E|Video standard (0 - PAL, 1 - NTSC, 2 - OLD NTSC, 3 - PALN)|
|000F|Future expansion|
|0010-0013|File data size (little endian; not including the header)|
|0014-xxxx|File data|

In TAP version 0, each data byte in the file data area represents the length of a single pulse determined by the following formula:

```data byte = pulse length (in seconds) * C64 clock frequency (in Hertz) / 8```

Therefore, a data value for a pulse length of 390µs would be:

```0.000390s * 985248Hz / 8 = 48 (0x30 in hex)```

A data value of 0x00 represents an "overflow" condition, any pulse length which is more than 255 * 8 clock cycles or 2078µs in length. Such a pulse length does not encode any data and normally only occurs in the pauses between a program and the following one on the same tape. However, the lengths of these pauses can be relevant for some fast loaders.

In TAP version 1, the data value of 0x00 has been re-coded to represent values greater than 255 * 8 clock cycles or values with a greater resolution (one clock cycle instead of eight). When a 0x00 is encountered, three bytes will follow which are the actual pulse length in C64 clock cycles. The three bytes are stored in little endian format (least significant byte first). For example, a data value for a pulse length of 2875µs would look like this:

```0.002875s * 985248Hz = 2832 (data bytes: 0x00, 0x10, 0x0B, 0x00)```

TAP Version 2 is an extension made by Markus Brenner for C16 tapes. It is version 1, but each value represents a halfwave, starting with a '0' to '1' transition. The time encoding doesn't change.

However, in practice, neither the storage of longer pulse lengths nor the higher resolution are relevant in most cases. That's why TAP version 0 is used for TapeDump64.

## Implementation
Without going too much into detail, the ATtiny measures the pulse lengths with its timer/counter B (TCB) in frequency measurement mode. The TCB continuously counts upwards at 8MHz until a falling edge is detected at the READ output of the Datasette. The current value of the counter is automatically saved, the counter is then reset to zero and restarted. In addition, an interrupt is triggered, in whose service routine the stored value is read out, divided by 64 and pushed into the output buffer for transmission to the PC.

As you can see, the calculation of the TAP values is based on a resulting clock frequency of 1MHz instead of 0.985248MHz for PAL or 1.022730MHz for NTSC. However, due to the resolution and the valid measuring range of the TAP format, this does not lead to any noteworthy deviations in the end result.

The tapedump Python script controls the device via three simple serial commands:

|Command|Function|Response|
|:-:|:-|:-|
|"i"|transmit indentification string|"TapeDump64\n"|
|"v"|transmit firmware version number|e.g. "v1.0\n"|
|"r"|read file from tape|raw data stream|

The raw datastream starts with a 0x00 as soon as <kbd>PLAY</kbd> on tape was pressed. Each valid pulse the device reads from the tape will be converted into a single non-zero byte which represents the pulse length according to TAP file format version 0 and immediately transmitted via UART. Invalid pulse lengths that would lead to TAP values of less than 16 are ignored. TAP values that would be greater than 255 are limited to 255 and later marked as a pulse overflow (byte 0x00) by the Python script. If <kbd>STOP</kbd> on tape was pressed or a timeout waiting for valid pulses occurs, the end of the stream is shown with a 0x00 byte. Afterwards, the 16-bit checksum, which is formed from the addition of all data bytes, is transmitted least significant byte first (little endian). Finally, the tape buffer overflow flag is transmitted as a single byte (0x00 means no overflow occured).

If <kbd>PLAY</kbd> was not pressed within the defined period of time at the beginning, a 0x01 is sent instead of a 0x00 and the procedure is ended.

The Python script itself writes the TAP file header and the received data bytes into the output file.

## Compiling and Uploading the Firmware
- Set the serial mode switch on the device to "UPDI". 
- Connect the device to a USB port of your PC.
- Use one of the following methods:

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
- Open the TapeDump64 sketch and click **Upload**.

### If using the makefile (Linux/Mac)
- Make sure you have installed the latest [avr-gcc toolchain](http://maxembedded.com/2015/06/setting-up-avr-gcc-toolchain-on-linux-and-mac-os-x/).
- Open a terminal.
- Navigate to the folder with the makefile and the Arduino sketch.
- Run `DEVICE=attiny202 make install` to compile, burn the fuses and upload the firmware (change DEVICE accordingly).

# Operating Instructions
- Set the serial mode switch on your TapeDump64 to "UART"
- Connect your TapeDump64 to your Commodore Datasette
- Connect your TapeDump64 to a USB port of your PC
- Execute the tapedump python script on your PC: `python tapedump.py outputfile.tap`
- Press <kbd>PLAY</kbd> on your Datasette
- The dumping is done fully automatically. It stops when the end of the cassette is reached, when there are no more signals on the tape for a certain time or when the <kbd>STOP</kbd> button on the Datasette is pressed.

![TapeDump64_pic2.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_TapeDump64/documentation/TapeDump64_pic2.jpg)
![TapeDump64_cli.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_TapeDump64/documentation/TapeDump64_cli.jpg)

# Checking, Cleaning and Converting TAP Files
The TAP file generated by TapeDump64 can be used directly in your favorite emulator or with the [Tapuino](https://github.com/wagiminator/C64-Collection/tree/master/C64_Tapuino), but perhaps you want to check it beforehand, optimize it or convert it into another format. This works perfectly with the command line tool [TAPClean](https://sourceforge.net/projects/tapclean/). To check the quality of the generated TAP image, use the following command:
`tapclean -t outputfile.tap`
Remember, a TAP file is almost never 100% clean, as pauses in particular are not always correctly recognized. You can optimize the quality of the file with the following command:
`tapclean -o outputfile.tap`
Slight deviations in the pulse lengths are compensated for and everything is adapted to the exact timing specifications. TAPClean not only knows the required pulse lengths of the standard Commodore coding, but also those of all common fast loaders.
If you want to convert the TAP file into a WAV audio file, then use the following command:
`tapclean -wav outputfile.tap`
There is also a graphical [front end](https://www.luigidifraia.com/software/) for TAPClean.

# References, Links and Notes
1. [TrueTape64 Project](https://github.com/francescovannini/truetape64)
2. [Tapuino Project](https://github.com/wagiminator/C64-Collection/tree/master/C64_Tapuino)
3. [How Commodore Tapes Work](https://wav-prg.sourceforge.io/tape.html)
4. [How TurboTape Works](https://www.atarimagazines.com/compute/issue57/turbotape.html)
5. [Analyzing C64 Tape Loaders](https://github.com/binaryfields/zinc64/blob/master/doc/Analyzing%20C64%20tape%20loaders.txt)
6. [Archiving C64 Tapes Correctly](https://www.pagetable.com/?p=1002)
7. [TAP File Format](https://ist.uwaterloo.ca/~schepers/formats/TAP.TXT)
8. [VICE Emulator File Formats](https://vice-emu.sourceforge.io/vice_17.html)
9. [TAPClean](https://sourceforge.net/projects/tapclean/)
10. [TAPClean Front End](https://www.luigidifraia.com/software/)
11. [Datasette Service Manual](https://www.vic-20.it/wp-content/uploads/C2N-1530-1531_service_manual.pdf)
12. [ATtiny Datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/ATtiny202-204-402-404-406-DataSheet-DS40002318A.pdf)
13. [CH340N Datasheet](https://datasheet.lcsc.com/lcsc/2101130932_WCH-Jiangsu-Qin-Heng-CH340N_C506813.pdf)
14. [MT3608 Datasheet](https://datasheet.lcsc.com/lcsc/1811151539_XI-AN-Aerosemi-Tech-MT3608_C84817.pdf)

![TapeDump64_pic3.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_TapeDump64/documentation/TapeDump64_pic3.jpg)
![TapeDump64_pic4.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_TapeDump64/documentation/TapeDump64_pic4.jpg)
![TapeDump64_pic5.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_TapeDump64/documentation/TapeDump64_pic5.jpg)

# License
![license.png](https://i.creativecommons.org/l/by-sa/3.0/88x31.png)

This work is licensed under Creative Commons Attribution-ShareAlike 3.0 Unported License. 
(http://creativecommons.org/licenses/by-sa/3.0/)
