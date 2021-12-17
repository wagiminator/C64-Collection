#!/usr/bin/env python
# -*- coding: utf-8 -*-

#    Magic Desk Compatible Cartridge Generator (c) 2013-2019  Žarko Živanov
#    Cartridge schematics and PCB design (c) 2013-2014 Marko Šolajić
#    E-mails: zzarko and msolajic at gmail

#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.

#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.

#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.

from __future__ import print_function
from __future__ import division

VERSION="3.0"

import copy
import array
import os
import glob
import sys
import re

if sys.version_info >= (3, 0):
    import configparser
else:
    import ConfigParser as configparser

#commodore 64 screen codes
SCR_CODES="""@abcdefghijklmnopqrstuvwxyz[|]|| !"#$%&'()*+,-./0123456789:;<=>?|ABCDEFGHIJKLMNOPQRSTUVWXYZ"""
SCR_DICT={SCR_CODES[pos]:pos for pos in range(len(SCR_CODES))}
#commodore 64 petascii codes
PETASCII_CODES="""0123456789|||||||abcdefghijklmnopqrstuvwxyz||||<|ABCDEFGHIJKLMNOPQRSTUVWXYZ"""
PETASCII_DICT={PETASCII_CODES[pos]:pos+48 for pos in range(len(PETASCII_CODES))}
MAX_MENU_ITEMS = 10+26*2
MAX_MENUS = 8

#cartridge size (64/128/256/512k/1024K, 0 - autosize)
cart_sizek = 0
cart_size = cart_sizek*1024
#directory for prg files
prg_directory  = "prg"
#cartridge menu program
cart_menu_file = "menu"
#finished cartridge bin file
cart_file_name = "compilation.bin"
#order of programs in CFG file
cart_order = []
#border color after program select
color_border = 14
#background color after program select
color_background = 6
#character color after program select
color_character = 14
#default menu help text
cart_help = "(Shift)CRSR: Scroll, Fn/Ret: Menu select"
#should menu wave
cart_wave = 1
#should menu have ROSS cartridge sound
cart_sound = 0

#returns array of screen codes
def to_screen(s):
    return [SCR_DICT[x] for x in s]

#returns the size of an open file
def flen(f):
    return os.fstat(f.fileno()).st_size

#adds one byte to array
def append_byte(arr,byte):
    arr.append(byte & 0xff)

#adds two bytes to array (lo/hi)
def append_word(arr,word):
    arr.append(word & 0xff)
    arr.append(word >> 0x08)

def append_text(bytearray, text, zero=False):
    text = to_screen(text)
    for c in text:
        append_byte(bytearray, c)
    if zero:
        append_byte(bytearray, 0)

def center_text(text, width=40):
    l = len(text)
    pos = (40 - l) // 2
    text = " "*pos + text + " "*(width-pos-len(text))
    return text

def error(s, usg=False):
    print("\nERROR!! {0}\n".format(s))
    if (usg):
        usage(1)
    else:
        exit(1)

def genitem(name, prg="", run=0, load=0, length=0, data=0):
    return {'name':name[:31], 'prg':prg, 'run':run, 'load':load, 'len': length, 'data':data, 'link': None}

def genmenu(title, order=None, spacing=0, width=0, height=0, x=0, y=0):
    return {'title':center_text(title), 'order':order, 'spacing':spacing, 'items':[],
        'width':width, 'height':height, 'x':x, 'y':y, 'offset':0}

def printmenus():
    print("\nMENUS:")
    for menuid in sorted(menus):
        menu = menus[menuid]
        print("MENU",menuid,menu['title'],"LEN",len(menu['items']),"W",menu['width'],"H",menu['height'])
        for i,item in enumerate(menu['items']):
            print("    ITEM",i,item['name'],"LINK",item['link'])

def usage(code=0):
    print("""Usage:
python {0}
 - scans the prg directory and places all prg files on cartridge
python {0} <cfg_file> (with or without extension)
 - reads cartridge configuration from CFG file

All programs for cartridge must be in prg format!

Create prg directory if it doesn't exist and place all prg files
for cartridge inside. To make specific program order, you can add
NNN_ prefix to prg files (N - 0-9). This prefix will be excluded
from name in menu. Or, you can use CFG file to define cartridge.
Sample CFG file is provided with installation.

Generated bin file can be burned to Magic Desk compatibile
eprom/cartridge, or converted to crt with VICE's cartconv:
cartconv -t md -i <bin_file> -o <crt_file>
""".format(sys.argv[0]))
    exit(code)

print(u"\nMagic Desk Cartridge Generator v%s\n(c) 2013-2019  Žarko Živanov" % VERSION)
print(u"\nMagic Desk Compatible Cartridge \n(c) 2013-2019  Marko Šolajić\n")

menus = {}
programs = {}
prgs_found = 0

if len(sys.argv) > 2: usage()
if len(sys.argv) > 1:
    #read configuration from cfg file
    cfgfile = sys.argv[1]
    if cfgfile[-4:] != ".cfg" and cfgfile[-4:] != ".CFG":
        cfgfile += ".cfg"
    if os.access(cfgfile, os.F_OK):
        print("\nReading configuration file %s ..." % cfgfile)
        if sys.version_info >= (3, 0):
            cfg = configparser.ConfigParser()
        else:
            cfg = configparser.SafeConfigParser()
        cfg.read(cfgfile)
        programs = cfg.sections()
        if cfg.has_section("cartridge"):
            print("Reading cartridge section ...")
            if cfg.has_option("cartridge","bin"):
                cart_file_name = cfg.get("cartridge","bin")
                if cart_file_name[-4:] != ".bin" and cart_file_name[-4:] != ".BIN":
                    cart_file_name += ".bin"
            if cfg.has_option("cartridge","size"):
                cart_sizek = cfg.getint("cartridge","size")
                if cart_sizek not in [64,128,256,512,1024]:
                    error("Wrong cartridge size, must be 64,128,256,512 or 1024")
            if cfg.has_option("cartridge","menu"):
                cart_menu_file = cfg.get("cartridge","menu")
                if cart_menu_file[-4:] == ".prg" or cart_menu_file[-4:] == ".PRG":
                    cart_menu_file = cart_menu_file[:-4]
            if cfg.has_option("cartridge","border"):
                color_border = cfg.getint("cartridge","border")
                if color_border not in range(16):
                    error("Border color must be 0-15")
            if cfg.has_option("cartridge","background"):
                color_background = cfg.getint("cartridge","background")
                if color_background not in range(16):
                    error("Background color must be 0-15")
            if cfg.has_option("cartridge","character"):
                color_character = cfg.getint("cartridge","character")
                if color_character not in range(16):
                    error("Character color must be 0-15")
            if cfg.has_option("cartridge","help"):
                cart_help = cfg.get("cartridge","help")
                if len(cart_help) > 40:
                    error("Cartridge help text must be 40 characters or less")
            if cfg.has_option("cartridge","wave"):
                cart_wave = int(cfg.get("cartridge","wave"))
            if cfg.has_option("cartridge","sound"):
                cart_sound = int(cfg.get("cartridge","sound"))
            programs.remove("cartridge")

        for m in range(1,MAX_MENUS+1):
            menu = "menu%d" % m
            if cfg.has_section(menu):
                print("Reading menu %d section ..." % m)
                title = ""
                order = ["{:03d}".format(i) for i in range(100*m,100*m+100)]
                spacing = width = height = x = y = 0
                if cfg.has_option(menu,"order"):
                    order = cfg.get(menu,"order")
                    order = order.split(",")
                    order = [ "%d%s" % (m, x) for x in order]
                if cfg.has_option(menu,"title"):
                    title = cfg.get(menu,"title")
                    if len(title) > 40:
                        error("Menu %d title must be 40 characters or less" % m)
                if cfg.has_option(menu,"spacing"):
                    spacing = int(cfg.get(menu,"spacing"))
                if cfg.has_option(menu,"width"):
                    width = int(cfg.get(menu,"width"))
                if cfg.has_option(menu,"height"):
                    height = int(cfg.get(menu,"height"))
                if cfg.has_option(menu,"x"):
                    x = int(cfg.get(menu,"x"))
                if cfg.has_option(menu,"y"):
                    y = int(cfg.get(menu,"y"))
                menus["%d" % m] = genmenu(title, order, spacing, width, height, x, y)
                programs.remove(menu)
        if len(menus) == 0:
            order = ["{:03d}".format(i) for i in range(100,200)]
            menus['1'] = genmenu("",order,0)

        for menuid in sorted(menus):
            menu = menus[menuid]
            print("Menu %s programs:" % menuid)
            for prgno in menu['order']:
                prgsec = "prg"+prgno
                if not cfg.has_section(prgsec):
                    continue
                prgname = cfg.get(prgsec,"file")
                if prgname[-4:] == ".prg" or prgname[-4:] == ".PRG":
                    prgname = prgname[:-4]
                if cfg.has_option(prgsec,"name"):
                    name = cfg.get(prgsec,"name")
                else:
                    name = prgname[:-4].replace("_"," ")
                if cfg.has_option(prgsec,"run"):
                    runaddr = cfg.get(prgsec,"run")
                    if runaddr[0] == '$': runaddr = runaddr.replace("$","0x")
                    runaddr = int(runaddr,0)
                else:
                    runaddr = 0
                menu['items'].append(genitem(name,prgname,runaddr))
                print("    %s" % name)
                programs.remove(prgsec)
                prgs_found += 1
    else:
        error("CFG file '{0}' not found.".format(cfgfile), True)

if len(menus) == 0 or prgs_found == 0:
    #collect all prg files in prg directory
    print("\nReading prg files from prg directory ...")
    prgList = glob.glob( os.path.join(prg_directory, '*.[pP][rR][gG]') )
    if len(prgList) == 0:
        error("No prg files found. Make sure to place them in prg directory.", True)
    prefix = re.compile("\A([0-9])([0-9]?[0-9]?)_(.*)")
    suffix = re.compile("(.*)_([0-9]+|0[xX][0-9a-fA-F]+)\Z")
    prgList.sort()
    print("Found prg files:")
    for prg in prgList:
        prgname = os.path.basename(prg)[:-4]
        regmatch = prefix.match(prgname)
        if regmatch:
            name = regmatch.group(3)
            menuid = regmatch.group(1)
        else:
            name = prgname
            menuid = '1'
        regmatch = suffix.match(name)
        if regmatch:
            name = regmatch.group(1)
            run = regmatch.group(2)
            if run[:2] == "0x":
                run = int(run, 16)
            else:
                run = int(run)
        else:
            run = 0
        print("    Menu %s: %s" % (menuid,name))
        name = name.replace("_"," ")
        if not menuid in menus:
            menus[menuid] = genmenu("")
        menus[menuid]['items'].append(genitem(name,prgname,run))

if len(programs) > 0:
    print("\nFollowing program sections were unused in cartridge:")
    for prgsec in sorted(programs):
        print("    [%s] %s" % (prgsec, cfg.get(prgsec,"file")) )

#list of prg names, used to detect repeated programs
load_list = []

#printmenus()

#load all programs
names_len = 0
for menuid in sorted(menus):
    menu = menus[menuid]
    menu['items'].append(genitem("Basic","",0xfce2))
    items_no = 0
    for i,item in enumerate(menu['items']):
        if item['prg'] == "": continue

        #check if the same file was already loaded
        found = None
        for ii,ll in enumerate(load_list):
            if item['prg'] == ll:
                found = ii
                break
        item['link'] = found

        prgfile=os.path.join(prg_directory,item['prg'])
        if os.access(prgfile+".PRG", os.F_OK):
            prgfile += ".PRG"
        else:
            prgfile += ".prg"
        prg = open(prgfile,"rb")
        temp = array.array('B')
        temp.fromfile(prg,flen(prg))
        addr = temp.pop(0) + 256*temp.pop(0)
        item['load'] = addr
        if item['run'] == 0:    # if there is no run address
            item['run'] = addr
        item['len'] = temp.buffer_info()[1]
        item['data'] = temp
        items_no += 1
        load_list.append(item['prg'])
    if items_no > MAX_MENU_ITEMS:
        error("Cartridge menu %s can have max %d programs, but %d supplied."
              % (menuid, MAX_MENU_ITEMS, items_no))

    width = menu['width']
    if width == 0:
        width = min( max( [ len(x['name'])+3 for x in menu['items'] ] ), 34 )
    else:
        width = min(menu['width'],34)
    menu['width'] = width

    height = menu['height']
    cheight = min( 20 if menu['spacing'] == 0 else 10, len(menu['items']) )
    if height == 0:
        height = cheight
    else:
        height = min(menu['height'],20 if menu['spacing'] == 0 else 10, cheight)
    menu['height'] = height

    x = menu['x']
    if x == 0:
        x = (40 - width - 3 - 3) // 2 + 1  #3 for borders, 3 for key display
    else:
        x = max(menu['x'],1)
    menu['x'] = x

    y = menu['y']
    if y == 0:
        y = (25 - height - menu['spacing']*(height-1) - 3) // 2      #3 for borders
    else:
        y = max(menu['y'],1)
    menu['y'] = y

    menu['offset'] = x + 40*y
    #length of all menu names
    nameslen = sum(len(x['name'])+1 for x in menu['items']) #+1 for null character
    menu['itemsoffset'] = names_len
    names_len += nameslen

#printmenus()

print("\nAssembling cartridge file ...")
#open cart menu file
if os.access(cart_menu_file+".PRG", os.F_OK):
    cart_menu_file += ".PRG"
else:
    if os.access(cart_menu_file+".prg", os.F_OK):
        cart_menu_file += ".prg"
    else:
        error("cartridge menu program '%s' not found." % cart_menu_file)
cart_prg = open(cart_menu_file,"rb")
#skip start address
cart_prg.seek(2)
cart_file = array.array('B')
cart_file.fromfile(cart_prg,flen(cart_prg)-2)
cart_prg.close()

#number of menus
menus_no = len(menus)
menunamesoffset = 2+5+8*(1+2+1+1+1+2+2)+40
menuitemsoffset = menunamesoffset + menus_no*40
#from ProgramTable to last menu item text
menudatasize = menuitemsoffset + names_len

#9 bytes for prgtable per item
tblsize = 9*sum( [ len(menus[menuid]['items']) for menuid in menus ] )

#calculate table addresses
table_data = array.array('B')
bank = 0
menuprglen = cart_file.buffer_info()[1]
#address of first program inside cartridge memory, starting at 0
crtaddress = menuprglen + menudatasize + tblsize
#address of first program inside C64 memory, starting at 0x8000
address = crtaddress + 0x8000
#start address of program table inside C64 memory
tbladdress =  menuprglen + menudatasize + 0x8000
menunamesaddress = menuprglen + menunamesoffset + 0x8000
menuitemsaddress = menunamesaddress + menus_no*40

if crtaddress > 0x2000:
    error("Program data has %d bytes, %d is the maximum.\nShorten program names or number of programs." % (crtaddress, 0x2000) )

#list of program table entries, used for repeated programs
tbl_list = []

print("\nCartridge memory map:\n%31s located at $%06x" % (cart_menu_file, 0))
print("%31s located at $%06x" % ("menu data",menuprglen))
print("%31s located at $%06x\n" % ("program table",tbladdress-0x8000))
for menuid in sorted(menus):
    menu = menus[menuid]
    for i,item in enumerate(menu['items']):
        prg_data = array.array('B')
        if item['link'] != None:
            print("%31s linked to previous instance" % item['name'])
            prg_data = tbl_list[item['link']]
            table_data.extend(prg_data)
            tbl_list.append(prg_data)
        else:
            append_byte(prg_data,bank)              #bank, 1 byte
            append_word(prg_data,address)           #address in bank, 2 bytes
            if item['prg'] != "":                   #length, 2 bytes
                print("%31s located at $%06x, run address:" % (item['name'],crtaddress), end=" ")
                length = item['data'].buffer_info()[1]
            else:
                length = 0
            append_word(prg_data,length)
            append_word(prg_data,item['load'])      #load address, 2 bytes
            if item['run'] == 2049:                 #run address, 2 bytes (0 - BASIC RUN)
                append_word(prg_data,0)
                print("BASIC RUN")
            else:
                append_word(prg_data,item['run'])
                if item['prg'] != "":
                    print("$%04x" % item['run'])
            table_data.extend(prg_data)
            tbl_list.append(prg_data)
            address += length                       #next program address
            crtaddress += length
            while address > 0x9fff:
                address -= 0x2000
                bank += 1

#assemble cartridge
#for details about various fields in here, check C64 assembler source
append_word(cart_file,tbladdress)           #program table address
append_byte(cart_file,color_border)         #border color
append_byte(cart_file,color_background)     #background color
append_byte(cart_file,color_character)      #character color
append_byte(cart_file,cart_wave)            #menu waving
append_byte(cart_file,cart_sound)           #menu sound

for menuid in sorted(menus):                #menu_items_no
    append_byte(cart_file,len(menus[menuid]['items']))
for i in range(8-len(menus)): append_byte(cart_file,0)

for menuid in sorted(menus):                #menu_offset
    append_word(cart_file,menus[menuid]['offset'])
for i in range(8-len(menus)): append_word(cart_file,0)

for menuid in sorted(menus):                #menu_width
    append_byte(cart_file,menus[menuid]['width'])
for i in range(8-len(menus)): append_byte(cart_file,0)

for menuid in sorted(menus):                #menu_height
    append_byte(cart_file,menus[menuid]['height'])
for i in range(8-len(menus)): append_byte(cart_file,0)

for menuid in sorted(menus):                #menu_spacing
    append_byte(cart_file,menus[menuid]['spacing'])
for i in range(8-len(menus)): append_byte(cart_file,0)

for idx, menuid in enumerate(sorted(menus)):    #menu_names
    append_word(cart_file,menunamesaddress + 40*idx)
for i in range(8-len(menus)): append_word(cart_file,0)

for menuid in sorted(menus):                #menu_items
    append_word(cart_file,menuitemsaddress+menus[menuid]['itemsoffset'])
for i in range(8-len(menus)): append_word(cart_file,0)

append_text(cart_file, center_text(cart_help))

for menuid in sorted(menus):
    append_text(cart_file, menus[menuid]['title'])

for menuid in sorted(menus):
    for item in menus[menuid]['items']:
        append_text(cart_file, item['name'], True)

cart_file.extend(table_data)

for menuid in sorted(menus):
    for item in menus[menuid]['items']:
        if (item['prg'] == "") or (item['link']): continue
        cart_file.extend(item['data'])

length = cart_file.buffer_info()[1]

#calculate size if set to 0
if cart_sizek == 0:
    cart_sizek = 64
    while (cart_sizek < 1024) and (length > cart_sizek*1024):
        cart_sizek *= 2
cart_size = cart_sizek*1024

if length > cart_size:
    larger = length - cart_size
    error("\nCartridge is %d bytes (%d blocks) larger than it should be."
          % (larger, larger // 254))

#pad to cartridge size
print("\nCartridge size %dk" % cart_sizek, end=" ")
if length < cart_size:
    unused = cart_size - length
    temp = array.array('B',[0xff]*unused)
    cart_file.extend(temp)
    print(", unused %d bytes / %d block(s)" % (unused , unused // 254))
else:
    print("")

#write cartridge to file
cartridge = open(cart_file_name,"wb")
cart_file.tofile(cartridge)
cartridge.close()
print("\nDone! Cartridge saved as '{0}'".format(cart_file_name))
print("\nIf needed, you can convert it to Magic Desk crt with cartconv from VICE:")
print("    cartconv -t md -i %s -o %s.crt\n" % (cart_file_name,cart_file_name[:-4]) )

