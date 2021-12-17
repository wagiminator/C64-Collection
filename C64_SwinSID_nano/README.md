# SwinSID nano
Replacement board for the legendary SID soundchip of the Commodore C64 based on the design by [Swinkels](http://www.swinkels.tvtom.pl/swinsid/).

- Project Video (YouTube): https://youtu.be/UqjzCAr04dE

![C64_SwinSID_nano_pic1.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_SwinSID_nano/documentation/C64_SwinSID_nano_pic1.jpg)

# Installing the Firmware
An AVR programmer is required to flash the firmware. A cheap and reliable programmer is e.g. [USBasp](https://aliexpress.com/wholesale?SearchText=usbasp). 

- Connect the programmer to the PC via USB and to the ICSP header on the board via an ICSP cable.
- Make sure you have installed [avrdude](https://learn.adafruit.com/usbtinyisp/avrdude).
- Open a terminal.
- Navigate to the software folder with the hex-files.
- Execute the following commands one after the other (if necessary replace "usbasp" with the programmer you use):
  ```
  avrdude -c usbasp -p m88p -U flash:w:SwinSID88_20120524.hex
  avrdude -c usbasp -p m88p -U lfuse:w:0xe0:m -U hfuse:w:0xdf:m
  ```

# Operating Instructions
- Set the jumpers on the board according to the SID IC version that is to be emulated (6581 or 8580).
- Open your switched off C64 and put the SwinSID nano into the IC socket provided for the SID. Pay attention to the correct orientation!
- Close your C64 and switch it on.
- Load a game and enjoy the unmatched sound.

![C64_SwinSID_nano_pic_a.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_SwinSID_nano/documentation/C64_SwinSID_nano_pic_a.jpg)
![C64_SwinSID_nano_pic_b.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_SwinSID_nano/documentation/C64_SwinSID_nano_pic_b.jpg)
