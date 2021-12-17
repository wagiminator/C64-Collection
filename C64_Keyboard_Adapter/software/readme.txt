avrdude -c usbasp -p m8 -U lfuse:w:0x9f:m -U hfuse:w:0xc8:m -U flash:w:main.hex
