avrdude -c usbasp -p m328p -U flash:w:tapuino.hex -U lfuse:w:0xff:m -U hfuse:w:0xdb:m -U efuse:w:0xfd:m

For more information and source code visit https://github.com/sweetlilmre/tapuino
