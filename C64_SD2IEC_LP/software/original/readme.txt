Flash firmware
==============
avrdude -c usbasp -p m644p -U flash:w:sd2iec-1.0.0-larsp-m644p.bin:a
avrdude -c usbasp -p m644p -U hfuse:w:0x91:m -U lfuse:w:0xef:m -U efuse:w:0xfd:m

Remove SD-Card before flashing! The device must not be connected to the C64 during flashing!

Source code: https://www.sd2iec.de
