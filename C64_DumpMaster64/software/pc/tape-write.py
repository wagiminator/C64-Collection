#!/usr/bin/env python3
# ===================================================================================
# Project:   DumpMaster64 - Python Script - Write TAP File to Tape
# Version:   v1.3
# Year:      2022
# Author:    Stefan Wagner
# Github:    https://github.com/wagiminator
# License:   http://creativecommons.org/licenses/by-sa/3.0/
# ===================================================================================
#
# Description:
# ------------
# Writes a tape image from TAP file to Datasette
#
# Dependencies:
# -------------
# - adapter (included in libs folder)
#
# Operating Instructions:
# -----------------------
# - Set the selector switch on your DumpMaster64 adapter to "UART"
# - Connect the adapter to your Commodore Datasette
# - Connect the adapter to a USB port of your PC
# - Execute this skript: python tape-write.py inputfile.tap
# - Press RECORD on your Datasette when prompted
# - The writing is done fully automatically. It stops when the file is recorded, 
#   the end of the cassette is reached or when the STOP button on the Datasette 
#   is pressed.


import sys
import os
from libs.adapter import *


# ===================================================================================
# Progress bar
# ===================================================================================

def progress(percent=0, width=50):
    left = width * percent // 100
    right = width - left
    sys.stdout.write('\r[' + '#' * left + '-' * right + '] ' + str(percent) + '%')
    sys.stdout.flush()


# ===================================================================================
# Main Function
# ===================================================================================

# Print Header
print('')
print('--------------------------------------------------')
print('DumpMaster64 - Python Command Line Interface v1.3')
print('(C) 2022 by Stefan Wagner - github.com/wagiminator')
print('--------------------------------------------------')


# Get and check command line arguments
try:
    fileName = sys.argv[1]
except:
    raise AdpError('Missing input file name')


# Establish serial connection
print('Connecting to DumpMaster64 ...')
dumpmaster = Adapter()
if not dumpmaster.is_open:
    raise AdpError('Adapter not found')
print('Adapter found on port', dumpmaster.port)
print('Firmware version:', dumpmaster.getversion())
print('')


# Open input file
print('Opening', fileName, 'for reading ...')
try:
    fileSize = os.stat(fileName).st_size
    f = open(fileName, 'rb')
except:
     raise AdpError('Failed to open ' + filename)


# Check file header
if fileSize < 20 or not f.read(12) == b'C64-TAPE-RAW':
    dumpmaster.close()
    f.close()
    raise AdpError('Wrong file header')


# Check TAP version
tapversion = f.read(1)[0]
if tapversion > 1:
    dumpmaster.close()
    f.close()
    raise AdpError('Unsupported TAP version')


# Check size of data aread
f.seek(16)
datasize = int.from_bytes(f.read(4), byteorder='little')
if not (datasize + 20) == fileSize:
    dumpmaster.close()
    f.close()
    raise AdpError('File size does not match header entry')


# Print TAP file information
print('File header: OK')
print('File size:   OK')
print('TAP version:', tapversion)


# Preparing data and store it in temp file
print('Preparing data ...')

try:
    t = open('dumpmaster.tmp', 'wb')
except:
    dumpmaster.close()
    f.close()
    raise AdpError('Failed to create temp file')

fcount  = datasize
tcount  = 0
taptime = 0

while fcount > 0:
    dataval = f.read(1)[0]
    fcount -= 1
    if dataval > 0:
        t.write(dataval.to_bytes(2, byteorder='little'))
        tcount  += 2
        taptime += dataval
    else: 
        if tapversion == 1:
            dataval  = int.from_bytes(f.read(3), byteorder='little') // 8
            taptime += dataval
            fcount  -= 3
            while dataval > 0:
                datalittle = dataval % 32640
                if(datalittle == 0):
                    datalittle = 32640
                t.write(datalittle.to_bytes(2, byteorder='little'))
                tcount  += 2
                dataval -= datalittle
        else:
            dataval = 32640
            t.write(dataval.to_bytes(2, byteorder='little'))
            tcount  += 2
            taptime += dataval

f.close()
t.close()
datasize = tcount
taptime  = taptime * 8 // 1000000 + 1
print('Estimated recording time:', taptime//60, 'min', taptime%60, 'sec')


# Send write command to DumpMaster64 and wait for RECORD pressed
print('PRESS RECORD & PLAY ON TAPE')
dumpmaster.sendcommand(CMD_WRITETAPE)

while 1:
    response = dumpmaster.read(1)
    if response:
        break
    sys.stdout.write('#')
    sys.stdout.flush()
print('\r', ' ' * 50, end='\r')
if response[0] > 0:
    dumpmaster.close()
    raise AdpError('Timeout waiting for RECORD')
else:
    print('OK')
    print('Start recording ...')


# Read data from temp file and write to tape
t = open('dumpmaster.tmp', 'rb')
checksum = 0

while 1:
    response = dumpmaster.read(1)
    if response:
        packsize = response[0]
        if packsize == 0:
            break
        if tcount <= 0:
            dumpmaster.write(b'\x00\x00')
        while packsize > 0 and tcount > 0:
            data = t.read(2)
            tcount -= 2
            dumpmaster.write(data)
            packsize -= 2
            checksum += data[0]
            checksum += data[1]
            if packsize > 0 and tcount <= 0:
                dumpmaster.write(b'\x00\x00')
            checksum %= 65536
        progress((datasize - tcount) * 100 // datasize)

print('')
testsum  = int.from_bytes(dumpmaster.read(2), byteorder='little')
underrun = dumpmaster.read(1)[0]
stopped  = dumpmaster.read(1)[0]


# Close temp file and serial connection
t.close()
dumpmaster.close()


# Validate data and checksum and print infos
if underrun > 0:
    sys.stderr.write('ERROR: Buffer underrun occured\n')
if tcount > 0 or stopped > 0:
    #raise AdpError('Recording was stopped before completion')
    sys.stderr.write('ERROR: Recording was stopped before completion\n')

print('Recording finished.')

if not checksum == testsum:
    raise AdpError('Checksum mismatch')

print('Checksum:       OK')
print('Buffer status:  OK')
print('Recording successful!')
print('PRESS STOP ON TAPE');
print('')


# Delete temp file and exit
try:
    os.remove('dumpmaster.tmp')
except:
    sys.stderr.write('ERROR: Failed to delete temp file\n')

sys.exit(0)
