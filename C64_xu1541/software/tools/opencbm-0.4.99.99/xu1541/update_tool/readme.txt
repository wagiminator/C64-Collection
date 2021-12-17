xu1541 update tool
------------------

This is 99.9% based on the avr usb bootloader by Thomas Fischl.
See www.fischl.de for details.

This version has been adopted to the xu1541 cable. See
http://www.harbaum.org/till/xu1541 for details.


Quickstart:

1. Install jumper switch between pins 9 and 10 of SV2. This
   is the pin pair of the 10 pin header facing the pcb corner.

2. Plug device into usb port. The yellow LED must light up and stay
   on. The LED will not stay on if the xu1541 you are trying to update
   is not equipped with a boot loader. Any xu1541 can be equipped with
   a boot loader, but this requires some special AVR programming
   adapter.

3. Use update tool with latest firmware from the firmware directory. E.g.:
   ./xu1541_update ../firmware/firmware-usbtiny.hex

   You have the choice between two different USB implementations for
   the AVR here. firmware-avrusb.hex is based on the avrusb stack
   while firmware-usbtiny.hex is based on the usbtiny stack. These two
   versions are functionally identical and it shouldn't matter which
   one you use. However if one version for one reason doesn't work
   for you you can try the other one. Please provide feedback if only
   one version is working for you.

4. Unplug device from USB

5. Remove jumper switch

6. Plug device into USB port. The yellow LED must light up
   for a fraction of a second and then stay off.

7. You are done ...
