#!/usr/bin/env python3
# ===================================================================================
# Project:   DiskBuddy64 - Python Script - Read Disk Directory
# Version:   v1.1
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
# - disktools (included in libs folder)
#
# Operating Instructions:
# -----------------------
# - Set the serial mode switch on your DiskBuddy64 adapter to "UART"
# - Connect the adapter to your floppy disk drive(s)
# - Connect the adapter to a USB port of your PC
# - Switch on your floppy disk drive(s)
# - Execute this skript:
#
# - python disk-dir.py [-h] [-d {8,9,10,11}]
#   optional arguments:
#   -h, --help                  show help message and exit
#   -d, --device                device number of disk drive (8-11, default=8)


import sys
import argparse
from libs.adapter import *
from libs.disktools import *


# Constants
FASTREAD_BIN = 'libs/fastread.bin'


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
if diskbuddy.uploadbin(FASTREAD_LOADADDR, FASTREAD_BIN) > 0:
    diskbuddy.close()
    raise AdpError('Failed to upload fast loader')


# Read BAM
dbam = BAM(diskbuddy.readblock(18, 0))
if not dbam.bam:
    diskbuddy.close()
    raise AdpError('Failed to read the BAM')


# Print disk title
print('')
print(dbam.getheader())


# Get number of free blocks
blocksfree = dbam.getblocksfree()


# Read file entries
track  = 18
sector = 1
while track > 0:
    block = diskbuddy.readblock(track, sector)
    if not block:
      diskbuddy.close()
      raise AdpError('Failed to read directory')

    track  = block[0]
    sector = block[1]
    ptr    = 0
    while ptr < 0xFF and block[ptr+0x02] > 0:
        line  = str(int.from_bytes(block[ptr+0x1E:ptr+0x20], byteorder='little')).ljust(5)
        line += '\"'
        line += (PETtoASC(PETdelpadding(block[ptr+0x05:ptr+0x15])) + '\"').ljust(19)
        line += FILETYPES[block[ptr+0x02] & 0x07]
        print(line.upper())
        ptr  += 0x20


# Finish all up
print(blocksfree, 'BLOCKS FREE.')
print('')
diskbuddy.close()
