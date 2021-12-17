Description
-----------

Magic Desk Cartridge Generator page:
https://bitbucket.org/zzarko/magic-desk-cartridge-generator/

This program can be used to generate Commodore 64 cartridges in
Magic Desk format, with C64 programs that can be selected from menu.

Cartridge hardware can be found at Marko Šolajić's GitHub page:
https://github.com/msolajic/c64-magic-desk-512k

All C64 programs must be one-file in PRG format (first 2 bytes are the
load address). Cartridge can be configured with CFG file (otherwise,
defaults will be used).

Menu programs can be configured inside CFG file (more advanced option).
They can also be configured just by placing prg files inside prg
directory. In that case:
    - name of the file will be used as menu name
    - you can make arbitrary order of files by placing N_ prefix, where
      N is a 1-3 digit number (prefix won't be a part of the menu name)
      Additionally, if you want multiple menus, you need to use 3-digit
      prefix in the form XYY_, where X is a menu number (1-8), and YY is
      placing inside menu. Numbers do not have to be successive.
    - programs in prg directory can also have suffix _N or _0xH where N
      is decimal, and H is hex number, and if present, that number will
      be used as run address (suffix won't be a part of the menu name)

You can try out cartridge generation with supplied files:

    python crtgen.py
this will create cartridge from all prg files in prg directory

    python crtgen.py compilation
this will create cartridge based on compilation.cfg file


For assembling C64 source, Kick Assembler 5.5 is needed (probably works
with earlier versions too, but I haven't tested).

If you want to test cartridge in VICE emulator, first convert it to
crt format:
    cartconv -t md -i compilation.bin -o compilation.crt
and then run x64 with cartcrt option:
    x64 -cartcrt compilation.crt
(or select compilation.crt from menu after starting x64)


As of VICE version 3.0, Magic Desk cartridge is supported with up to
1MB configurations (Thanks VICE team!!!).

Python code was tested on Linux and Windows, with Python 2 and 3.
C64 code was tested in VICE 3.3 and on a real PAL and NTSC C64.
If you find bugs in the code, please report as bitbucket issue, or
to one of our e-mails.

There are some programs that won't work when started from cartridge,
but for now we do not know why. Two such programs are in "nonworking"
directory. If someone has some insights abot why they don't work, or
if someone finds another one that doesn't work, please contact us.

Contact e-mails: msolajic and zzarko at gmail



Archive contents
----------------

- menu.asm
    ASM source code for cartridge menu program, Kick Assembler 5.5
- menu.prg
    alredy built menu program
- compilation.cfg and prg directory
    sample configuration file and prg files to assemble a cartridge
- crtgen.py
    Python program that links prg files with menu program and generates
    bin file that can be burned to eprom, or converted to CRT file for
    emulation (using VICE's cartconv)
    It should run on any system that has Python and required modules
    installed (see beginning of crtgen.py for required modules)
- readme.txt
    this file
- gpl.txt
    GNU General Public License version 3



Copyright
---------

Cartridge schematics and PCB design (c) 2013-2019 Marko Šolajić
menu.prg, menu.asm and crtgen.py (c) 2013-2019 Žarko Živanov

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.


Programs in prg and nonworking directories are copyrighted by their respective owners.



Sample configuration file with all descriptions
-----------------------------------------------

;optional section; contains cartridge information - if ommited, defaults will be used
[cartridge]

;cartridge size in kilobytes; can be 64, 128, 256, 512, 1024 or 0
;optional - if ommited, 0 will be used (cartridge size will be calculated on the go)
size=128

;name of generated cartridge file
;optional - if ommited, 'compilation.bin' will be used; extension can also be ommited
bin=compilation.bin

;menu file (code that executes menu in cartridge)
;optional - if ommited, 'menu.prg' will be used; extension can also be ommited
menu=menu.prg

;text for last line on screen
;optional - if ommited, default text will be used
help=(Shift)CRSR: Scroll, Fn/Ret: Menu select

;should menu wave
;optional - if ommited, menu will wave
wave=1

;should menu have Ross cartridge sound (popular ex-Yugoslav cartridges, VICE ID 23)
;optional - if ommited, menu will not have sound
sound=1

;colors for border, background and characters, these will be set before
;starting selected program
;optional - if ommited, defaults will be used
border=14
background=6
character=14

;same program can be at several places in one or more menus, but only one copy
;will be put into cartridge, and its all menu items will point to that copy

;optional section; can be from menu1 to menu8, contains menu information
;if ommited, program sections for menu 1 will be used (prg100, prg101, ...)
[menu1]

;menu title
;optional - if ommited, there will be no title
title=Magic Desk cartridge generator

;order of programs in menu
;optional - if ommited, ascending order of program sections will be applied
order=05,01,02,03,04,06,07,08,09,10,11,12,13,14,15
;for menu 1, 1 is appended to prg number, for menu 2, 2 is appended, and so on
;the line above will select sections prg105, prg101, prg102 and so on

;menu width
;optional - if ommited, optimal value will be used
width=25

;menu height
;optional - if ommited, optimal value will be used
height=7

;menu x coordinate (must be 1 or higher)
;optional - if ommited, menu will be centered horizontally
x=7

;menu y coordinate (must be 1 or higher)
;optional - if ommited, menu will be centered vertically
y=3

;should menu items have an empty line between them
;optional - if ommited, there will be no epmpty lines
spacing=1

;when manually setting menu position and size, make sure that menu will fit on screen!


;program section, name should be 'prgMNN', where M is menu number and NN is 00-99
;if no program sections are found, prg directory will be scaned
[prg101]
;filename in prg directory; can be with or without prg extension
file=cruncher.prg
;name of program in menu; optional - if ommited, filename will be used
name=Cruncher AB 1.2
;run address; optional - if ommited or set to 0, the loading address
;from prg file will be used; can be decimal or hexadecimal format
;if program starts at 2049 ($0801), it will be started with Basic's RUN command
run=$0801

;you don't have to use successive numbers for program sections
[prg105]
file=pro_text



Configuration examples
----------------------

1. Just cartridge section, no help text shown, turn on sound, turn off waving

[cartridge]
size=256
help=
sound=1
wave=0


2. Just menu section with menu title and x,y position.
   Menu will be filed with all programs in prg directory

[menu1]
title=Disk tools
x=3
y=3


3. More detailed definition

[cartridge]
sound=1
wave=1

[menu1]
title=Arcade games

[menu2]
title=Logic games
order=25,20
y=2

[prg100]
file=commando
name=Commando

[prg105]
file=pacman
name=Pac-Man

[prg220]
file=tetris.prg
name=Tetris

[prg225]
file=sweep.prg
name=Mine sweep


Changes since v2.0
------------------
menu.prg
  - source transferred to Kick Assembler 5.5
  - complete rewrite of display code
  - added support for up to 8 menus, which can be selected with Fn keys
    or cycled through using Return
  - significantly lowered memory needed for tables and menu text, total
    of 256 programs can be put on cartridge (actually, I haven't tested
    that much, but if someone tries, let me know)
  - waving of the menu can be turned off
  - menu can produce sound used in popular ex-Yugoslav cartridges made by Ross

crtgen.py
  - rewrite of menu tables generation
  - autosize of cartridge if size is set to zero (this is the default)
  - configurable menu position, size and item spacing
  - same program can be on several menus, only one copy will be in cartridge
    (but only if it is the same filename; same programs under different
    filenames aren't recognised)
  - configurable help text
  - prg sections in CFG file are now with 3 digits instead of 2
    (adjustments must be made to configurations from v2.0)
  - more checks of configuration file

