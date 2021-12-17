avrdude -c usbasp -p m88p -U flash:w:SwinSID88_20120524.hex
avrdude -c usbasp -p m88p -U lfuse:w:0xe0:m -U hfuse:w:0xdf:m

Source code: 
