# C64 Keyboard Adapter
C64 keyboard to USB adapter based on the design and firmware by [Mikkel Holm Olsen](https://symlink.dk/projects/c64key/). Use the original keyboard with you favourite emulator!

![C64_Keyboard_Adapter_pic1.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_Keyboard_Adapter/documentation/C64_Keyboard_Adapter_pic1.jpg)

# Installing the Firmware
An AVR programmer is required to flash the firmware. A cheap and reliable programmer is e.g. [USBasp](https://aliexpress.com/wholesale?SearchText=usbasp). 

- Connect the programmer to the PC via USB and to the ICSP header on the board via an ICSP cable.
- Make sure you have installed [avrdude](https://learn.adafruit.com/usbtinyisp/avrdude).
- Open a terminal.
- Navigate to the software folder with the hex-file.
- Execute the following command (if necessary replace "usbasp" with the programmer you use):
  ```
  avrdude -c usbasp -p m8 -U lfuse:w:0x9f:m -U hfuse:w:0xc8:m -U flash:w:main.hex
  ```

# Operating Instructions
- Connect the C64 keyboard to the respective pin header on the adapter.
- Connect the adapter via a USB cable to your PC.
- The adapter is automatically recognized as a keyboard by the operating system. No driver installation is necessary.
