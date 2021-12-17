What is this?
-------------

This is slightly modified version of Zzarko's Magic Desk Cartridge Generator, aims to be used with single prg file and load it automatically, avoiding any menu interaction. 

Original work can be found here: 
https://bitbucket.org/zzarko/magic-desk-cartridge-generator/

Cartridge hardware can be found at Marko Šolajić's GitHub page:
https://github.com/msolajic/c64-magic-desk-512k



How to use?
-----------

Note: Replacing "menu.prg" will be more than enough for existing Magic Desk Cartridge Generator users.  

Place your favourite prg file inside "prg" directory then simply run "python crtgen.py" in command line. "Compilation.bin" file will be created as output.

To create *.crt file,
cartconv -t md -i compilation.bin -o yourfilename.crt
command may be used with VICE's cartconv utility.

Avoid using multiple prg files.
Avoid using any configuration file.
For any further information please read original readme file.
 

File Structure
--------------

-> /prg/ 
-----Folder to put your *.prg file

-> crtgen.py
-----Linking your *.prg file with menu.prg

-> gpl.txt
-----GNU General Public License version 3

-> menu.asm(modified)
-----Source code for menu.prg with a few modifications

-> menu.prg(modified)
-----Compiled menu file(see above). 

-> readme(original).txt
-----Original readme file from Zzarko's latest release.

-> readme.txt
-----My own humble readme file. 



Modification Notes
------------------

This is a very simple/primitive modification that includes:
1- Menu functions commented out so menu will not show up at startup.  

2- Presses "1" on keyboard automatically so first *.prg file linked will launch. 

Cannot guarantee this version works stable in every case. Use at your own risk. Currently working on a more user friendly version which detects prg file count(single or multiple) and behaves properly in both scenarios.   

Feel free to contact me via feandreu at gmail.com for any issues.


  

Copyright(*excerpt from original readme file)
---------

  
Cartridge schematics and PCB design (c) 2013-2020 Marko Šolajić
menu.prg, menu.asm and crtgen.py (c) 2013-2020 Žarko Živanov

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
