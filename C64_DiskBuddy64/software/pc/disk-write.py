#!/usr/bin/env python3
# ===================================================================================
# Project:   DiskBuddy64 - Python Script - Write Disk Image from D64 File
# Version:   v1.0
# Year:      2022
# Author:    Stefan Wagner
# Github:    https://github.com/wagiminator
# License:   http://creativecommons.org/licenses/by-sa/3.0/
# ===================================================================================
#
# Description:
# ------------
# Writes disk image from D64 file
#
# Dependencies:
# -------------
# - adapter (included in libs folder)
#
# Operating Instructions:
# -----------------------
# - python disk-write.py [-h] [-b] [-d {8,9,10,11}] -f FILE
#   optional arguments:
#   -h, --help            show help message and exit
#   -b, --bamonly         only write blocks with BAM entry (recommended)
#   -d, --device          device number of disk drive (8-11, default=8)
#   -f FILE, --file FILE  input file (*.d64)
# - Example: python disk-write.py -b -f game.d64

import sys
import os
import time
import argparse
from libs.adapter import *


# ===================================================================================
# Main Function
# ===================================================================================


# Variables
tracks  = 35
bamcopy = 0
ramaddr = 0x0500
device  = 8


# Print Header
print('')
print('--------------------------------------------------')
print('DiskBuddy64 - Python Command Line Interface v1.0')
print('(C) 2022 by Stefan Wagner - github.com/wagiminator')
print('--------------------------------------------------')


# Get and check command line arguments
parser = argparse.ArgumentParser(description='Simple command line interface for DiskBuddy64')
parser.add_argument('-b', '--bamonly', action='store_true', help='only write blocks with BAM entry (recommended)')
parser.add_argument('-d', '--device', choices={8, 9, 10, 11}, type=int, default=8, help='device number of disk drive (default=8)')
parser.add_argument('-f', '--file', required=True, help='input file (*.d64)')

args = parser.parse_args(sys.argv[1:])
if args.bamonly:  bamcopy  = 1
device   = args.device
filename = args.file


# Establish serial connection
print('Connecting to DiskBuddy64 ...')
diskbuddy = Adapter()

if not diskbuddy.is_open:
    raise AdpError('Adapter not found')

print('Adapter found on port', diskbuddy.port)
print('Firmware version:', diskbuddy.getversion())


# Check if IEC device ist present
if not diskbuddy.checkdevice(device):
    diskbuddy.close()
    raise AdpError('IEC device ' + str(device) + ' not found')


# Upload fast loader to disk drive RAM
print('Uploading fast loader ...')
if diskbuddy.uploadbin(ramaddr, 'libs/fastwrite.bin') > 0:
    diskbuddy.close()
    sys.exit(1)


# Open input file
print('Opening', filename, 'for reading ...')
try:
    filesize = os.stat(filename).st_size
    f = open(filename, 'rb')
except:
    diskbuddy.close()
    raise AdpError('Could not open ' + filename)


# Check input file
if not filesize == getfilepointer(tracks + 1, 0):
    f.close()
    diskbuddy.close()
    raise AdpError('Wrong input file size')

# Read BAM if necessary
if bamcopy > 0:
    print('Reading BAM of input file ...')
    f.seek(getfilepointer(18, 0))
    fbam = BAM(f.read(256))


# Write disk
print('Writing disk ...')
starttime = time.time()
for track in range(1, tracks + 1):
    secnum      = getsectors(track)
    sectors     = [x for x in range(secnum)]

    # Cancel sectors without BAM entry
    if bamcopy > 0 and track < 36:
        for x in range(secnum):
            if fbam.blockisfree(track, x): sectors.remove(x)

    # Send command to disk drive, if there's something to write on track
    seclen = len(sectors)
    if seclen > 0:
        if diskbuddy.startfastiec('w', ramaddr, track, sectors) > 0:
              f.close()
              diskbuddy.close()
              sys.exit(1)

    # Write track
    trackline = ('Track ' + str(track) + ':').ljust(10) + '['
    sys.stdout.write(trackline + '-' * seclen + '0' * (secnum - seclen) + ']')
    sys.stdout.write('\r' + trackline)
    sys.stdout.flush()
    diskbuddy.timeout = 3
    for sector in sectors:
        f.seek(getfilepointer(track, sector))
        counter = 256;
        while counter > 0:
            requested = diskbuddy.read(1);
            if not requested:
                print('')
                f.close()
                diskbuddy.close()
                raise AdpError('Writing to disk failed')
            diskbuddy.write(f.read(requested[0]))
            counter -= requested[0]
        sys.stdout.write('#')
        sys.stdout.flush()
    if seclen > 0:  diskbuddy.read(1);
    print('')


# Finish all up
duration = time.time() - starttime
print('Done.')
print('Duration:', round(duration), 'seconds')
print('')
f.close()
diskbuddy.close()
