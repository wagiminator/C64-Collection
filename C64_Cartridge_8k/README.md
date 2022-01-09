# Simple 8k Cartridge for C64
![C64_Cartridge_8k_pic1.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_Cartridge_8k/documentation/C64_Cartridge_8k_pic1.jpg)

# Usable Types of (E)EPROMs
Most 8k (E)EPROMs should work. They usually have one of the following identifiers in their names: 2764, 2864, 27C64, or 28C64.

# Burning a BIN File to the (E)EPROM 
There are numerous programming devices/EPROMers. The [MiniPRO TL866](https://aliexpress.com/wholesale?SearchText=MiniPro+TL866) is used in these instructions. This is available for around €50 and supports almost all (E)EPROMs.

![MiniPro_TL866.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_Cartridge_8k/documentation/MiniPro_TL866.jpg)

After starting the MiniPRO software, search in the IC database for the (E)EPROM used. The (E)EPROM must now be correctly inserted in the socket. A click on the "Information" button shows how this must be positioned. Pay attention to the notch and to pin 1. If an EPROM (not EEPROM) is used, the following must be observed: Make sure that the EPROM is empty. A "blank check" provides information about this. An EPROM that is not empty must first be erased. EEPROMs, on the other hand, can simply be overwritten. Open the cartridge image (you can find some in the software folder) and confirm with "OK". Now click on the "P" button and confirm with "Program". The (E)EPROM will now be burned. So that the EPROM does not lose its data, the glass pane should be sealed with an opaque adhesive tape. An EEPROM does not have this window and there is also no risk of the data being lost.

The (E)EPROM is now ready for use and can be plugged into the socket of the cartridge. Important: Pay attention to the alignment of the notch on the (E)EPROM! It must match the notch on the module PCB.

The MiniPRO software is only available for Windows, but it also runs smoothly on Linux and Mac with WINE. Alternatively, the open source command line tool [minipro](https://gitlab.com/DavidGriffith/minipro/) is available for these operating systems.

# Erasing an EPROM
In contrast to EEPROMS (Electrically Erasable Programmable Read-only Memory), EPROMs cannot be overwritten with new data, but must first be erased with an EPROM eraser. This sends ultraviolet radiation into the EPROM. This erases the memory, which can then be rewritten. A cheap [EPROM eraser](https://aliexpress.com/wholesale?SearchText=eprom+eraser) is sufficient. Don't forget to remove any UV cover from the EPROM. The erasing process takes 10-15 minutes in most cases. The EPROM can then be written to again.

![EPROM_Eraser.jpg](https://raw.githubusercontent.com/wagiminator/C64-Collection/master/C64_Cartridge_8k/documentation/EPROM_Eraser.jpg)

# Converting a CRT File into a BIN File
Sometimes the image file is in CRT format. The [VICE](https://sourceforge.net/projects/vice-emu/) Commodore emulator includes a command line tool that can convert CRT files into BIN files. To do this, open a terminal and enter the following command:

```
cartconv -i input.crt -o output.bin
```

# Setting the DIP Switches
Depending on the start address of the program on the (E)EPROM, both switches must either be set to "$8000" or to "$E000". If you are unsure, just give it a try. But remember that the C64 has to be switched off if you change the switch positions.

# Operating Instruction
- Switch off your C64 if you haven't already done so.
- Insert the cartridge into the expansion slot of your C64.
- Switch on your C64.
- The program on the cartridge starts automatically.

# References, Links and Notes
1. [Making a C64 Cartridge](http://blog.worldofjani.com/?p=879)
2. [Create 8K and 16K Cartridges (german)](https://mingos-commodorepage.com/tutorials/vic20cartridges2.php)
3. [How to make an 8k Cartridge for your Commodore 64](http://swut.net/c64cart-howto.html)
4. [minipro Software](https://gitlab.com/DavidGriffith/minipro/)
5. [C64 Games](https://www.c64games.de/)
