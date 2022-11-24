# 1541-II Power Guard
The old "power bricks" that supply the 1541-II/1571-II/1581 floppy disk drives lose their reliability over time and can then damage the drive through overvoltage. The 1541-II Power Guard is connected between the power supply and the floppy drive, it monitors the voltages on the 5V and 12V rails and immediately disconnects if an overvoltage occurs. It thus makes a valuable contribution to protecting your old hardware.

![1541II_PowerGuard_pic8.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_1541II_PowerGuard/documentation/1541II_PowerGuard_pic8.jpg)

# Hardware
## Schematic
![1541II_PowerGuard_wiring.png](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_1541II_PowerGuard/documentation/1541II_PowerGuard_wiring.png)

## PCB Versions
Two PCB versions are available. One version allows a 4-pin DIN input socket to be soldered directly onto the PCB, while the other has solder pads for both the input and output lines so that cable-type connectors can be soldered here.

![1541II_PowerGuard_pic1.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_1541II_PowerGuard/documentation/1541II_PowerGuard_pic1.jpg)

## Working Principle
The [LM393](https://www.onsemi.com/pdf/datasheet/lm393-d.pdf) dual comparators constantly compare the input voltages (5V and 12V) via a voltage divider with the [LM4040](https://datasheet.lcsc.com/lcsc/1912111437_Diodes-Incorporated-LM4040B25FTA_C460725.pdf) reference voltage (2.5V). As soon as a divided voltage is above the reference voltage, the power supply is disconnected via a P-channel MOSFET.

![1541II_PowerGuard_hardware.png](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_1541II_PowerGuard/documentation/1541II_PowerGuard_hardware.png)

## Setting the Voltage Limits
The upper voltage limit for the 5V power rail is set via the resistors R1 and R2 of the voltage divider:

$$V_{LIMIT} = V_{REF} \times \frac{R1 + R2}{R2} = 2.5V \times \frac{12kΩ + 10kΩ}{10kΩ} = 5.5V$$

The upper voltage limit for the 12V power rail is set via the resistors R7 and R8 of the voltage divider:

$$V_{LIMIT} = V_{REF} \times \frac{R7 + R8}{R8} = 2.5V \times \frac{12kΩ + 3kΩ}{3kΩ} = 12.5V$$

## MOSFET Selection
Two P-channel MOSFETs are required to switch the power connections. In principle, all logic level P-channel MOSFETs are suitable, provided they meet the following conditions:
- must be in the SOT-23-3 package,
- Drain-Source Voltage must be at least $V_{DS} = 12V$,
- Gate-Source Voltage must be at least $V_{GS} = 12V$,
- Continuous Drain Current must be at least $I_D = 2A$,
- Gate Threshold Voltage must not exceed $V_{GS(th)} = 2.5V$.

The values referred to are usually given as negative values for P-channel MOSFETs. In addition, care should be taken to ensure that the On-Resistance $(R_{DS(on)})$ at a Gate-Source Voltage of -4.5V is as low as possible.

An excellent choice is the [G7P03L](https://datasheet.lcsc.com/lcsc/2009211935_GOFORD-G7P03L_C840062.pdf) with an on-resistance of 23mΩ@-4.5V. If this is not available, an [AO3401A](https://datasheet.lcsc.com/lcsc/2007171935_HUASHUO-AO3401A_C700954.pdf) with an on-resistance of 64mΩ@-4.5V will also do.

# Building Instructions
![1541II_PowerGuard_socket.png](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_1541II_PowerGuard/documentation/1541II_PowerGuard_socket.png)
![1541II_PowerGuard_pic5.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_1541II_PowerGuard/documentation/1541II_PowerGuard_pic5.jpg)
![1541II_PowerGuard_pic6.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_1541II_PowerGuard/documentation/1541II_PowerGuard_pic6.jpg)
![1541II_PowerGuard_pic7.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_1541II_PowerGuard/documentation/1541II_PowerGuard_pic7.jpg)
![1541II_PowerGuard_pic9.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_1541II_PowerGuard/documentation/1541II_PowerGuard_pic9.jpg)

# Operating Instructions
Before use, test the 1541-II Power Guard for proper operation using a variable power supply and a multimeter. Connect the input of the 1541-II Power Guard to the power supply. When both LEDs light up, the output can be connected to the floppy disk drive and it can be switched on safely.

# References, Links and Notes
1. [LM393 Datasheet](https://www.onsemi.com/pdf/datasheet/lm393-d.pdf)
2. [LM4040 Datasheet](https://datasheet.lcsc.com/lcsc/1912111437_Diodes-Incorporated-LM4040B25FTA_C460725.pdf)
3. [G7P03L Datasheet](https://datasheet.lcsc.com/lcsc/2009211935_GOFORD-G7P03L_C840062.pdf)
4. [AO3401A Datasheet](https://datasheet.lcsc.com/lcsc/2007171935_HUASHUO-AO3401A_C700954.pdf)
5. [4-Pin DIN Connectors on AliExpress](https://aliexpress.com/wholesale?SearchText=4+pin+din+connector)
6. [USB-C to 1541-II Power Adapter](https://github.com/wagiminator/C64-Collection/tree/master/C64_1541II_PowerAdapter)

# License
![license.png](https://i.creativecommons.org/l/by-sa/3.0/88x31.png)

This work is licensed under Creative Commons Attribution-ShareAlike 3.0 Unported License. 
(http://creativecommons.org/licenses/by-sa/3.0/)
