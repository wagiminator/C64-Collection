#!/usr/bin/env python3
# ===================================================================================
# Project:   DiskBuddy64 - Python Script - Read Disk Directory
# Version:   v1.5
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
FASTLOAD_BIN = 'libs/fastload.bin'


# Get and check command line arguments
parser = argparse.ArgumentParser(description='Simple command line interface for DumpMaster64')
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
if diskbuddy.uploadbin(FASTLOAD_LOADADDR, FASTLOAD_BIN) > 0:
    diskbuddy.close()
    raise AdpError('Failed to upload fast loader')


# Read directory
blocks = bytes()
if diskbuddy.startfastload(18, 0) > 0:
    diskbuddy.close()
    raise AdpError('Failed to start disk operation')

diskbuddy.timeout = 4
while 1:
    block = diskbuddy.getblock(256)
    diskbuddy.timeout = 1
    if not block:
        diskbuddy.close()
        raise AdpError('Failed to read directory')
    blocks += block
    if block[0] == 0:
        break

directory = Dir(blocks)


# Print disk title
print('')
print(directory.header)


# Print files
for file in directory.filelist:
    line  = str(file['size']).ljust(5)
    line += '\"'
    line += (file['name'] + '\"').ljust(19)
    line += file['type']
    if file['locked']: line += '<'
    if not file['closed']: line += '*'
    print(line.upper())


# Print free blocks
print(directory.blocksfree, 'BLOCKS FREE.')

# Finish all up
print('')
diskbuddy.close()
