# C64 Joystick to USB Adapter
With this simple adapter you can use your old C64 9-pin joysticks, paddles and mice on your modern PC.

![C64_Joystick_Adapter_pic1.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_Joystick_Adapter/documentation/C64_Joystick_Adapter_pic1.jpg)

# Hardware
## Schematic
![C64_Joystick_Adapter_wiring.png](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_Joystick_Adapter/documentation/C64_Joystick_Adapter_wiring.png)

## CH552G 8-bit USB Device Microcontroller
The CH552G is a low-cost, enhanced E8051 core microcontroller compatible with the MCS51 instruction set. It has an integrated USB 2.0 controller with full-speed data transfer (12 Mbit/s) and supports up to 64 byte data packets with integrated FIFO and direct memory access (DMA). The CH552G has a factory built-in bootloader so firmware can be uploaded directly via USB without the need for an additional programming device.

## Commodore 64 9-pin Control Port
The Commodore 64 features two control ports on its rightmost side. These ports are designed to allow for the connection of peripheral devices such as joysticks, paddles, mice, light pens, and sound digitizers. This feature of the Commodore 64 made it an incredibly versatile and customizable system, capable of being adapted to meet the needs of a wide range of users.

The connector used for these ports is a 9 pin d-sub male connector. This was a widely used connector in the 1980s, particularly for devices such as joysticks. The versatility of this connector meant that users could easily plug in a wide range of peripheral devices, allowing them to customize their computing experience to suit their individual needs.

Pin	| Signal | Remarks
--- | ------ | -------
1	  | Joy 0	 | joystick up / right mouse button
2	  | Joy 1	 | joystick down
3	  | Joy 2	 | joystick left / paddle fire x
4	  | Joy 3	 | joystick right / paddle fire y
5	  | Pot Y	 | paddle/mouse pot y
6	  | But A  | joystick fire / left mouse button
7	  | + 5V	 | max. 100 mA
8	  | GND    | Ground
9   | Pot X  | paddle/mouse pot x

![C64_Joystick_Adapter_pic2.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_Joystick_Adapter/documentation/C64_Joystick_Adapter_pic2.jpg)

# Software
## Joystick Firmware
With this firmware, all digital joysticks for the C64 can be used on the PC via USB. Mice in joystick mode can also be used as game controllers. The adapter is recognized as a standard HID game controller, so no driver installation is necessary.

## Paddle Firmware
With this firmware, old C64 paddles can be used as USB game controllers on modern PCs. The paddles are registered as analog joysticks, where the x-axis is controlled by one paddle and the y-axis by the other. The adapter is recognized as a standard HID game controller, so no driver installation is necessary.

## Mouse Firmware
With this firmware, old C64 mice both in joystick mode (e.g. Commodore 1350) and in proportional mode (e.g. Commodore 1351) can be used as USB mice on modern PCs. The adapter is recognized as a standard HID mouse, so no driver installation is necessary.

# Compiling and Installing Firmware
## Preparing the CH55x Bootloader
### Installing Drivers for the CH55x Bootloader
On Linux you do not need to install a driver. However, by default Linux will not expose enough permission to upload your code with the USB bootloader. In order to fix this, open a terminal and run the following commands:

```
echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="4348", ATTR{idProduct}=="55e0", MODE="666"' | sudo tee /etc/udev/rules.d/99-ch55x.rules
sudo service udev restart
```

For Windows, you need the [CH372 driver](http://www.wch-ic.com/downloads/CH372DRV_EXE.html). Alternatively, you can also use the [Zadig Tool](https://zadig.akeo.ie/) to install the correct driver. Here, click "Options" and "List All Devices" to select the USB module, and then install the libusb-win32 driver. To do this, the board must be connected and the CH55x must be in bootloader mode.

### Entering CH55x Bootloader Mode
A brand new chip starts automatically in bootloader mode as soon as it is connected to the PC via USB. Once firmware has been uploaded, the bootloader must be started manually for new uploads. To do this, the board must first be disconnected from the USB port and all voltage sources. Now press the BOOT button and keep it pressed while reconnecting the board to the USB port of your PC. The chip now starts again in bootloader mode, the BOOT button can be released and new firmware can be uploaded within the next couple of seconds.

## Compiling and Uploading using the makefile
### Installing SDCC Toolchain for CH55x
Install the [SDCC Compiler](https://sdcc.sourceforge.net/). In order for the programming tool to work, Python3 must be installed on your system. To do this, follow these [instructions](https://www.pythontutorial.net/getting-started/install-python/). In addition [pyusb](https://github.com/pyusb/pyusb) must be installed. On Linux (Debian-based), all of this can be done with the following commands:

```
sudo apt install build-essential sdcc python3 python3-pip
sudo pip install pyusb
```

### Compiling and Uploading Firmware
- Open a terminal.
- Navigate to the folder with the makefile. 
- Connect the board and make sure the CH55x is in bootloader mode. 
- Run ```make flash``` to compile and upload the firmware. 
- If you don't want to compile the firmware yourself, you can also upload the precompiled binary. To do this, just run ```python3 ./tools/chprog.py firmware.bin```.

## Compiling and Uploading using the Arduino IDE
### Installing the Arduino IDE and CH55xduino
Install the [Arduino IDE](https://www.arduino.cc/en/software) if you haven't already. Install the [CH55xduino](https://github.com/DeqingSun/ch55xduino) package by following the instructions on the website.

### Compiling and Uploading Firmware 
- Open the .ino file in the Arduino IDE.
- Go to **Tools -> Board -> CH55x Boards** and select **CH552 Board**.
- Go to **Tools** and choose the following board options:
  - **Clock Source:**   12 MHz (internal)
  - **Upload Method:**  USB
  - **USB Settings:**   USER CODE /w 0B USB RAM
- Connect the board and make sure the CH55x is in bootloader mode. 
- Click **Upload**.

# Operating Instructions
- Make sure you have installed the correct firmware for your C64 input device.
- Connect your C64 input device via the 9-pin connector to the board.
- Connect the board via USB to your PC. It should be detected as a HID device.

![C64_Joystick_Adapter_pic3.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_Joystick_Adapter/documentation/C64_Joystick_Adapter_pic3.jpg)

# References, Links and Notes
1. [EasyEDA Design Files](https://oshwlab.com/wagiminator)
2. [CH551/552 Datasheet](http://www.wch-ic.com/downloads/CH552DS1_PDF.html)
3. [SDCC Compiler](https://sdcc.sourceforge.net/)

# License
![license.png](https://i.creativecommons.org/l/by-sa/3.0/88x31.png)

This work is licensed under Creative Commons Attribution-ShareAlike 3.0 Unported License. 
(http://creativecommons.org/licenses/by-sa/3.0/)
