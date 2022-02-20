#!/usr/bin/env python3
# ===================================================================================
# Project:   DumpMaster64 - Python Script - Read Disk Image to D64 File
# Version:   v1.3.1
# Year:      2022
# Author:    Stefan Wagner
# Github:    https://github.com/wagiminator
# License:   http://creativecommons.org/licenses/by-sa/3.0/
# ===================================================================================
#
# Description:
# ------------
# Reads disk image to D64 file
#
# Dependencies:
# -------------
# - adapter (included in libs folder)
# - disktools (included in libs folder)
#
# Operating Instructions:
# -----------------------
# - Set the serial mode switch on your DumpMaster64 adapter to "UART"
# - Connect the adapter to your floppy disk drive(s)
# - Connect the adapter to a USB port of your PC
# - Switch on your floppy disk drive(s)
# - Execute this skript:
#
# - python disk-read.py [-h] [-x] [-b] [-d {8,9,10,11}] [-i INTER] [-f FILE]
#   optional arguments:
#   -h, --help                    show help message and exit
#   -x, --extend                  read disk with 40 tracks
#   -b, --bamonly                 only read blocks with BAM entry (recommended)
#   -d, --device                  device number of disk drive (8-11, default=8)
#   -i INTER, --interleave INTER  sector interleave (default=4)
#   -f FILE, --file FILE          output file (default=output.d64)
#
# - Example: python disk-read.py -b -f game.d64


import sys
import time
import argparse
from libs.adapter import *
from libs.disktools import *


# Constants and variables
FASTREAD_BIN   = 'libs/fastread.bin'
FASTUPLOAD_BIN = 'libs/fastupload.bin'
trackgap = 5
tracks = 35


# Print Header
print('')
print('--------------------------------------------------')
print('DumpMaster64 - Python Command Line Interface v1.3')
print('(C) 2022 by Stefan Wagner - github.com/wagiminator')
print('--------------------------------------------------')


# Get and check command line arguments
parser = argparse.ArgumentParser(description='Simple command line interface for DumpMaster64')
parser.add_argument('-x', '--extend', action='store_true', help='read disk with 40 tracks')
parser.add_argument('-b', '--bamonly', action='store_true', help='only read blocks with BAM entry (recommended)')
parser.add_argument('-d', '--device', choices={8, 9, 10, 11}, type=int, default=8, help='device number of disk drive (default=8)')
parser.add_argument('-i', '--interleave', type=int, default=4, help='sector interleave (default=4)')
parser.add_argument('-f', '--file', default='output.d64', help='output file (default=output.d64)')

args = parser.parse_args(sys.argv[1:])
if args.extend: tracks = 40
bamcopy    = args.bamonly
device     = args.device
filename   = args.file
interleave = args.interleave
if interleave < 1 or interleave > 16: interleave = 4


# Establish serial connection
print('Connecting to DumpMaster64 ...')
dumpmaster = Adapter()
if not dumpmaster.is_open:
    raise AdpError('Adapter not found')


# Check if IEC device ist present
print('Connecting to IEC device', device, '...')
if not dumpmaster.checkdevice(device):
    dumpmaster.close()
    raise AdpError('IEC device ' + str(device) + ' not found')


# Upload fast loader to disk drive RAM
print('Uploading fast loader ...')
if dumpmaster.uploadbin(FASTUPLOAD_LOADADDR, FASTUPLOAD_BIN) > 0 \
or dumpmaster.fastuploadbin(FASTREAD_LOADADDR, FASTREAD_BIN) > 0:
    dumpmaster.close()
    raise AdpError('Failed to upload fast loader')


# Create output file
print('Opening', filename, 'for writing ...')
try:
    f = open(filename, 'wb')
except:
    dumpmaster.close()
    raise AdpError('Failed to open ' + filename)


# Fill output file with default values
for track in range(1, tracks + 1):
    for sector in range(getsectors(track)):
        f.write(b'\x4B' + b'\x01' * 255)


# Read BAM if necessary
if bamcopy:
    print('Reading and checking BAM ...')
    dbam = BAM(dumpmaster.readblock(18, 0))
    if not dbam.bam:
        f.close()
        dumpmaster.close()
        raise AdpError('Failed to read the BAM')


# Read disk
print('Reading from disk ...')
print('')
copied = 0
errors = 0
starttime = time.time()
for track in range(1, tracks + 1):
    secnum  = getsectors(track)
    sectors = [x for x in range(secnum)]
    seclist = []

    # Cancel sectors without BAM entry
    if bamcopy and track < 36:
        for x in range(secnum):
            if dbam.blockisfree(track, x): sectors.remove(x)

    # Optimize order of sectors for speed
    sector  = trackgap * (track - 1)
    counter = len(sectors)
    while counter:
        sector %= secnum
        while not sector in sectors:
            sector += 1
            if sector >= secnum: sector = 0
        seclist.append(sector)
        sectors.remove(sector)
        sector  += interleave
        counter -= 1

    # Send command to disk drive, if there's something to read on track
    seclen = len(seclist)
    if seclen > 0:
        if dumpmaster.startfastread(track, seclist) > 0:
            f.close()
            dumpmaster.close()
            raise AdpError('Failed to start disk operation')

    # Read track
    trackline = list('\r' + ('Track ' + str(track) + ':').ljust(10) + '[' + '-' * secnum + ']')
    for x in range(secnum):
        if not x in seclist: trackline[x + 12] = '0'
    sys.stdout.write(''.join(trackline))
    sys.stdout.flush()
    dumpmaster.timeout = 3
    for sector in seclist:
        f.seek(getfilepointer(track, sector))
        block = dumpmaster.getblockgcr()
        if not block:
            print('')
            f.close()
            dumpmaster.close()
            raise AdpError('Failed to read from disk')
        if not len(block) == 256:
            trackline[sector + 12] = 'R'
            errors += 1
        else:
            f.write(block)
            trackline[sector + 12] = '#'
            copied += 1
        sys.stdout.write(''.join(trackline))
        sys.stdout.flush()
        dumpmaster.timeout = 1
    print('')


# Finish all up
dumpmaster.executememory(MEMCMD_SETTRACK18)
duration = time.time() - starttime
f.close()
dumpmaster.close()

print('')
print(copied, 'blocks copied.')
print('Errors:  ', errors, 'bad sectors')
print('Duration:', round(duration), 'seconds')
print('')
