#!/usr/bin/env python3
# ===================================================================================
# Project:   DiskBuddy64 - Python Script - Read Disk Directory
# Version:   v1.0
# Year:      2022
# Author:    Stefan Wagner
# Github:    https://github.com/wagiminator
# License:   http://creativecommons.org/licenses/by-sa/3.0/
# ===================================================================================
#
# Description:
# ------------
# Reads disk directory
#
# Dependencies:
# -------------
# - adapter (included in libs folder)
#
# Operating Instructions:
# -----------------------
# - python disk-dir.py [-h] [-d {8,9,10,11}]
#   optional arguments:
#   -h, --help                  show help message and exit
#   -d, --device                device number of disk drive (8-11, default=8)


import sys
import argparse
from libs.adapter import *

# Variables
ramaddr = 0x0500
device  = 8
filetypes = ['DEL', 'SEQ', 'PRG', 'USR', 'REL']


# ===================================================================================
# Main Function
# ===================================================================================


# Get and check command line arguments
parser = argparse.ArgumentParser(description='Simple command line interface for DiskBuddy64')
parser.add_argument('-d', '--device', choices={8, 9, 10, 11}, type=int, default=8, help='device number of disk drive (default=8)')
args = parser.parse_args(sys.argv[1:])
device = args.device


# Establish serial connection
diskbuddy = Adapter()
if not diskbuddy.is_open:
    raise AdpError('Adapter not found')


# Check if IEC device ist present
if not diskbuddy.checkdevice(device):
    diskbuddy.close()
    raise AdpError('IEC device ' + str(device) + ' not found')


# Upload fast loader to disk drive RAM
if diskbuddy.uploadbin(ramaddr, 'libs/fastread.bin') > 0:
    diskbuddy.close()
    sys.exit(1)


# Read BAM
dbam = BAM(diskbuddy.readblock(ramaddr, 18, 0))
if not dbam.bam:
    diskbuddy.close()
    sys.exit(1)


# Print disk title
print('')
print(dbam.getheader())


# Get number of free blocks
blocksfree = dbam.getblocksfree()


# Read file entries
track  = 18
sector = 1
while track > 0:
    block = diskbuddy.readblock(ramaddr, track, sector)
    if not block:
      diskbuddy.close()
      sys.exit(1)

    track  = block[0]
    sector = block[1]
    ptr    = 0
    while ptr < 0xFF and block[ptr+0x02] > 0:
        line  = str(int.from_bytes(block[ptr+0x1E:ptr+0x20], byteorder='little')).ljust(5)
        line += '\"'
        line += (PETtoASC(PETdelpadding(block[ptr+0x05:ptr+0x15])) + '\"').ljust(19)
        line += filetypes[block[ptr+0x02] & 0x07]
        print(line.upper())
        ptr  += 0x20
    

# Finish all up
print(blocksfree, 'BLOCKS FREE.')
print('')
diskbuddy.close()
