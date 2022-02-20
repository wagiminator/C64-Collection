#!/usr/bin/env python3
# ===================================================================================
# Project:   DiskBuddy64 - Python Script - Format Disk
# Version:   v1.5.1
# Year:      2022
# Author:    Stefan Wagner
# Github:    https://github.com/wagiminator
# License:   http://creativecommons.org/licenses/by-sa/3.0/
# ===================================================================================
#
# Description:
# ------------
# Formats a disk
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
# - python disk-format.py [-h] [-x] [-n] [-c] [-v] [-d {8,9,10,11}] [-t TITLE] [-i IDENT]
#   optional arguments:
#   -h, --help                  show help message and exit
#   -x, --extend                format 40 tracks
#   -n, --nobump                do not bump head
#   -c, --clear                 clear (demagnetize) disk (recommended for new disks)
#   -v, --verify                verify each track after it is formatted
#   -d, --device                device number of disk drive (8-11, default=8)
#   -t TITLE, --title TITLE     disk title (max 16 characters, default=commodore)
#   -i IDENT, --ident IDENT     disk ident (2 characters, default=64)
#
# - Example: python disk-format.py -t games -i a7


import sys
import time
import argparse
from libs.adapter import *
from libs.disktools import *


# Constants and variables
FASTFORMAT_BIN = 'libs/fastformat.bin'
FASTUPLOAD_BIN = 'libs/fastupload.bin'
tracks = 35
bump   = 1
demag  = 0
verify = 0


# Print Header
print('')
print('--------------------------------------------------')
print('DiskBuddy64 - Python Command Line Interface v1.5')
print('(C) 2022 by Stefan Wagner - github.com/wagiminator')
print('--------------------------------------------------')


# Get and check command line arguments
parser = argparse.ArgumentParser(description='Simple command line interface for DiskBuddy64')
parser.add_argument('-x', '--extend', action='store_true', help='format 40 tracks')
parser.add_argument('-n', '--nobump', action='store_true', help='do not bump head')
parser.add_argument('-c', '--clear', action='store_true', help='clear (demagnetize) disk (recommended for new disks)')
parser.add_argument('-v', '--verify', action='store_true', help='verify each track after it is formatted')
parser.add_argument('-d', '--device', choices={8, 9, 10, 11}, type=int, default=8, help='device number of disk drive (default=8)')
parser.add_argument('-t', '--title', default='commodore', help='disk title (max 16 characters, default=commodore)')
parser.add_argument('-i', '--ident', default='64', help='disk ident (2 characters, default=aa)')

args = parser.parse_args(sys.argv[1:])
if args.extend: tracks = 40
if args.nobump: bump   = 0
if args.clear:  demag  = 1
if args.verify: verify = 1
device = args.device
diskName  = ASCtoPET(args.title.lower())
diskIdent = ASCtoPET(args.ident.lower())

if len(diskName) < 1 or len(diskName) > 16 or not len(diskIdent) == 2:
    raise AdpError('Unsupported disk title or disk ID')


# Establish serial connection
print('Connecting to DiskBuddy64 ...')
diskbuddy = Adapter()
if not diskbuddy.is_open:
    raise AdpError('Adapter not found')


# Check if IEC device ist present
print('Connecting to IEC device', device, '...')
if not diskbuddy.checkdevice(device):
    diskbuddy.close()
    raise AdpError('IEC device ' + str(device) + ' not found')


# Upload fast formatter to disk drive RAM
print('Uploading fast formatter ...')
if diskbuddy.uploadbin(FASTUPLOAD_LOADADDR, FASTUPLOAD_BIN) > 0 \
or diskbuddy.fastuploadbin(FASTFORMAT_LOADADDR, FASTFORMAT_BIN) > 0:
        diskbuddy.close()
        raise AdpError('Failed to upload fast formatter')


# Format the disk
if diskbuddy.startfastformat(tracks, bump, demag, verify, diskName, diskIdent) > 0:
    diskbuddy.close()
    raise AdpError('Failed to start disk operation')

starttime = time.time()
sys.stdout.write('Formatting: [' + '-' * (tracks) + ']\r')
sys.stdout.write('Formatting: [')
sys.stdout.flush()
diskbuddy.timeout = 4
for x in range(tracks + 1):
    progress = diskbuddy.read(1)
    if not progress or progress[0] > 0:
        print('')
        diskbuddy.close()
        raise AdpError('Failed to format the disk')
    if x > 0:
        sys.stdout.write('#')
        sys.stdout.flush()

print('')


# Finish all up
duration = time.time() - starttime
print(tracks, 'tracks formatted.')
print('Duration:', round(duration), 'seconds')
print('')
diskbuddy.close()
