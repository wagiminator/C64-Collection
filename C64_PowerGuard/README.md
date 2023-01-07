# C64 Power Guard
The old "power bricks" that supply the Commodore C64 lose their reliability over time and can then damage the device through overvoltage. The C64 Power Guard is connected between the power supply and the C64, it monitors the voltage and current on the 5V DC rail and immediately disconnects if an overvoltage or overcurrent occurs. It thus makes a valuable contribution to protecting your old hardware.

![C64_PowerGuard_pic4.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_PowerGuard/documentation/C64_PowerGuard_pic4.jpg)

# Hardware
## Schematic
![C64_PowerGuard_wiring.png](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_PowerGuard/documentation/C64_PowerGuard_wiring.png)

## PCB Versions
Two PCB versions are available. One version allows a 7-pin DIN input socket to be soldered directly onto the PCB, while the other has solder pads for both the input and output lines so that cable-type connectors can be soldered here.

![C64_PowerGuard_pic1.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_PowerGuard/documentation/C64_PowerGuard_pic1.jpg)

## Working Principle
The [LM393](https://www.onsemi.com/pdf/datasheet/lm393-d.pdf) comparator constantly compares the input voltage via a voltage divider with the [LM4040](https://datasheet.lcsc.com/lcsc/1912111437_Diodes-Incorporated-LM4040B25FTA_C460725.pdf) reference voltage (2.5V). As soon as the divided voltage is above the reference voltage, the power supply is disconnected via a P-channel MOSFET. The upper voltage limit for the 5V DC power rail is set via the resistors R1 and R2 of the voltage divider:

$$V_{LIMIT} = V_{REF} \times \frac{R1 + R2}{R2} = 2.5V \times \frac{12kΩ + 10kΩ}{10kΩ} = 5.5V$$

The 1.5A resettable fuse interrupts the power connection if an excessive load occurs, e.g. in the event of a short circuit. As a rule, a C64 does not consume more than 1.5A on the 5V DC power rail, but if required (e.g. when operating additional modules on the C64), a higher current value of the fuse can also be selected. If a fuse is not required, a 0Ω resistor can be soldered in instead or the contacts can simply be shorted with a solder bridge.

![C64_PowerGuard_hardware.png](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_PowerGuard/documentation/C64_PowerGuard_hardware.png)

## MOSFET Selection
A P-channel MOSFETs is required to switch the DC power connection. In principle, all logic level P-channel MOSFETs are suitable, provided they meet the following conditions:
- must be in the SOT-23-3 package,
- Drain-Source Voltage must be at least $V_{DS} = 8V$,
- Gate-Source Voltage must be at least $V_{GS} = 8V$,
- Continuous Drain Current must be at least $I_D = 2A$,
- Gate Threshold Voltage must not exceed $V_{GS(th)} = 2.5V$.

The values referred to are usually given as negative values for P-channel MOSFETs. In addition, care should be taken to ensure that the On-Resistance $(R_{DS(on)})$ at a Gate-Source Voltage of -4.5V is as low as possible.

An excellent choice is the [G7P03L](https://datasheet.lcsc.com/lcsc/2009211935_GOFORD-G7P03L_C840062.pdf) with an on-resistance of 23mΩ@-4.5V. If this is not available, an [AO3401A](https://datasheet.lcsc.com/lcsc/2007171935_HUASHUO-AO3401A_C700954.pdf) with an on-resistance of 64mΩ@-4.5V will also do.

# Building Instructions
![C64_PowerGuard_socket.png](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_PowerGuard/documentation/C64_PowerGuard_socket.png)
![C64_PowerGuard_pic5.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_PowerGuard/documentation/C64_PowerGuard_pic5.jpg)
![C64_PowerGuard_pic6.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_PowerGuard/documentation/C64_PowerGuard_pic6.jpg)
![C64_PowerGuard_pic11.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_PowerGuard/documentation/C64_PowerGuard_pic11.jpg)

# Operating Instructions
Before use, test the C64 Power Guard for proper operation using a variable power supply and a multimeter. Connect the input of the C64 Power Guard to the power supply. When the POWER GOOD LED lights up, the output can be connected to the C64 and it can be switched on safely.

# References, Links and Notes
1. [LM393 Datasheet](https://www.onsemi.com/pdf/datasheet/lm393-d.pdf)
2. [LM4040 Datasheet](https://datasheet.lcsc.com/lcsc/1912111437_Diodes-Incorporated-LM4040B25FTA_C460725.pdf)
3. [G7P03L Datasheet](https://datasheet.lcsc.com/lcsc/2009211935_GOFORD-G7P03L_C840062.pdf)
4. [AO3401A Datasheet](https://datasheet.lcsc.com/lcsc/2007171935_HUASHUO-AO3401A_C700954.pdf)
5. [7-Pin DIN Connectors](https://aliexpress.com/wholesale?SearchText=7+pin+din+connector)
6. [1541-II Power Guard](https://github.com/wagiminator/C64-Collection/tree/master/C64_1541II_PowerGuard)

![C64_PowerGuard_pic2.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_PowerGuard/documentation/C64_PowerGuard_pic2.jpg)
![C64_PowerGuard_pic3.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_PowerGuard/documentation/C64_PowerGuard_pic3.jpg)

# License
![license.png](https://i.creativecommons.org/l/by-sa/3.0/88x31.png)

This work is licensed under Creative Commons Attribution-ShareAlike 3.0 Unported License. 
(http://creativecommons.org/licenses/by-sa/3.0/)
