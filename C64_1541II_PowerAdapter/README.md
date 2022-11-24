# USB-C to 1541-II Power Adapter
The old "power bricks" that supply the 1541-II/1571-II/1581 floppy disk drives lose their reliability over time and can then damage the drive through overvoltage. There are definitely newer power supplies to buy or you can build one yourself. However, with the increasing spread of USB Type-C PD power adapters, which can supply different voltages and high currents, it may make more sense to use one of these to power the 1541-II. The 1541-II PowerAdapter makes just that possible.

![1541II_PowerAdapter_pic1.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_1541II_PowerAdapter/documentation/1541II_PowerAdapter_pic1.jpg)

# Hardware
## Schematic
![1541II_PowerAdapter_wiring.png](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_1541II_PowerAdapter/documentation/1541II_PowerAdapter_wiring.png)

## CH224K USB-PD Power Receiving Chip
The CH224K is a USB PD power receiving protocol chip, which integrates PD3.0/2.0, BC1.2 and other fast charging protocols, automatically detects VCONN and analog E-Mark chips, supports up to 100W power, and has built-in PD communication module. It also integrates output voltage detection internally to support overheating and overvoltage protection. It features:

- 4V to 22V input voltage
- PD3.0/2.0, BC1.2 and other fast charging protocols
- USB Type-C PD, positive and negative insertion detection and automatic switching
- E-Mark simulation, automatically detects VCONN, supports 100W power PD request
- requested voltage can be dynamically adjusted through a variety of methods
- high integration of single chip, simplified peripheral and low cost
- built-in over-voltage and over-temperature protection module

The CH224K in this application has been set to request 12V from the power supply.

## SD8942 Synchronous Step-Down Converter
The SD8942 is a fully integrated, high–efficiency 2A synchronous rectified step-down converter. The SD8942 operates at high efficiency over a wide output current load range.
This device offers two operation modes, PWM control and PFM Mode switching control, which allows a high efficiency over the wider range of the load. The SD8942 requires a minimum number of readily
available standard external components. It feature:

- 4.5V to 16V input voltage
- high efficiency: up to 96%
- 600kHz frequency operation
- 2A output current
- no schottky diode required
- over current protection
- thermal shutdown
- inrush current limit and soft start

The SD8942 in this application provides the 5V for the disk drive. The SD8942 can be replaced by MT2492.

# Building Instructions
![1541II_PowerAdapter_pic3.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_1541II_PowerAdapter/documentation/1541II_PowerAdapter_pic3.jpg)
![1541II_PowerAdapter_socket2.png](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_1541II_PowerAdapter/documentation/1541II_PowerAdapter_socket2.png)
![1541II_PowerAdapter_pic4.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_1541II_PowerAdapter/documentation/1541II_PowerAdapter_pic4.jpg)
![1541II_PowerAdapter_pic5.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_1541II_PowerAdapter/documentation/1541II_PowerAdapter_pic5.jpg)

# Operating Instructions
Use a USB-C power supply that can provide at least 12V/1.5A (18W). Plug the power supply into an outlet. Connect the PowerAdapter to the power supply with a USB-C cable. Wait for the POWER-GOOD LED on the PowerAdapter to light up. Plug the PowerAdapter's 4-pin DIN connector into the 1541-II Floppy Disk Drive and turn it on.

# References, Links and Notes
1. [CH224K Datasheet](https://datasheet.lcsc.com/lcsc/2204251615_WCH-Jiangsu-Qin-Heng-CH224K_C970725.pdf)
2. [SD8942 Datasheet](https://datasheet.lcsc.com/lcsc/1808081634_SHOUDING-SD8942_C250795.pdf)
3. [4-Pin DIN Connectors on AliExpress](https://aliexpress.com/wholesale?SearchText=4+pin+din+connector)

# License
![license.png](https://i.creativecommons.org/l/by-sa/3.0/88x31.png)

This work is licensed under Creative Commons Attribution-ShareAlike 3.0 Unported License. 
(http://creativecommons.org/licenses/by-sa/3.0/)
