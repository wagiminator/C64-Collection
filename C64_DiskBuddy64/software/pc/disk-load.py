#!/usr/bin/env python3
# ===================================================================================
# Project:   DiskBuddy64 - Python Script - Read File(s) from Disk
# Version:   v1.5
# Year:      2022
# Author:    Stefan Wagner
# Github:    https://github.com/wagiminator
# License:   http://creativecommons.org/licenses/by-sa/3.0/
# ===================================================================================
#
# Description:
# ------------
# Reads file(s) from floppy disk to PRG files on PC
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
# - python disk-load.py [-h] [-d {8,9,10,11}]
#   optional arguments:
#   -h, --help                  show help message and exit
#   -d, --device                device number of disk drive (8-11, default=8)


import sys
import time
import argparse
from libs.adapter import *
from libs.disktools import *


# Constants
FASTLOAD_BIN = 'libs/fastload.bin'


# ===================================================================================
# Progress bar
# ===================================================================================

def progress(percent=0, width=50):
    left = width * percent // 100
    right = width - left
    sys.stdout.write('\r[' + '#' * left + '-' * right + '] ' + str(percent) + '%')
    sys.stdout.flush()


# ===================================================================================
# Read file
# ===================================================================================

def readFile(fileindex):
    # Create output file
    filename  = cleanstring(directory.filelist[fileindex]['name']) + '.prg'
    blocksize = directory.filelist[fileindex]['size']
    print('Opening', filename, 'for writing ...')
    try:
        f = open(filename, 'wb')
    except:
        diskbuddy.close()
        raise AdpError('Failed to open ' + filename)

    # Start read operation
    print('Transfering \"' + directory.filelist[fileindex]['name'] 
                           + '\" to \"' + filename + '\" ...')
    track  = directory.filelist[fileindex]['track']
    sector = directory.filelist[fileindex]['sector']
    starttime = time.time()
    if diskbuddy.startfastload(track, sector) > 0:
        f.close()
        diskbuddy.close()
        raise AdpError('Failed to start disk operation')

    # Read file from disk to output file
    written = 0;
    diskbuddy.timeout = 4
    while 1:
        block = diskbuddy.getblock(256)
        diskbuddy.timeout = 1
        if not block:
            f.close()
            diskbuddy.close()
            print('')
            raise AdpError('Failed to read from disk')
        written += 1
        progress(written * 100 // blocksize)
        if block[0] == 0:
            f.write(block[2:block[1]+1])
            break
        f.write(block[2:])

    duration = time.time() - starttime
    f.close()
    print('')
    print(blocksize, 'blocks successfully transfered in', round(duration), 'seconds.')
    print('')


# ===================================================================================
# Main Function
# ===================================================================================

# Get and check command line arguments
parser = argparse.ArgumentParser(description='Simple command line interface for DiskBuddy64')
parser.add_argument('-d', '--device', choices={8, 9, 10, 11}, type=int, default=8, help='device number of disk drive (default=8)')
args = parser.parse_args(sys.argv[1:])
device = args.device


# Print Header
print('')
print('--------------------------------------------------')
print('DiskBuddy64 - Python Command Line Interface v1.5')
print('(C) 2022 by Stefan Wagner - github.com/wagiminator')
print('--------------------------------------------------')


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


# Upload fast loader to disk drive RAM
print('Uploading fast loader ...')
if diskbuddy.uploadbin(FASTLOAD_LOADADDR, FASTLOAD_BIN) > 0:
    diskbuddy.close()
    raise AdpError('Failed to upload fast loader')


# Read directory
print('Reading directory ...')
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


# Print files
print('')
print('Disk title:', directory.title)
indices = list()
index = 0
counter = 1
for file in directory.filelist:
    if file['type'] == 'PRG' and file['size'] > 0:
        print(('(' + str(counter) + ')').ljust(5), end='')
        print(('\"' + file['name'] + '\"').ljust(22), end='')
        if counter % 2 == 0:
            print('')
        indices.append(index)
        counter += 1
    index += 1
if (counter - 1) % 2 > 0:
    print('')
print('(A)  ALL FILES             (Q)  QUIT')


# Get selection and take proper action
print('')
selection = input('Your choice? ').upper()
print('')

if selection == 'Q':
    diskbuddy.close()
    sys.exit(0)

if selection == 'A':
    for index in indices: readFile(index)
    diskbuddy.close()
    sys.exit(0)

try:
    number = int(selection)
    if number < 1 or number >= counter: raise Exception
except:
    raise AdpError('Invalid choice')

index = indices[number- 1]
readFile(index)
diskbuddy.close()
