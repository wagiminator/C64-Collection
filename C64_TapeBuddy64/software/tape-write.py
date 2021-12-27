#!/usr/bin/env python3
# ===================================================================================
# Project:   TapeBuddy64 - Python Script for Command Line Interface - WRITE
# Version:   v1.0
# Year:      2021
# Author:    Stefan Wagner
# Github:    https://github.com/wagiminator
# License:   http://creativecommons.org/licenses/by-sa/3.0/
# ===================================================================================
#
# Description:
# ------------
# TapeBuddy64 is a simple and inexpensive adapter that can interface a Commodore
# Datasette to your PC via USB in order to read from or write to tapes.
#
# References:
# -----------
# - https://ist.uwaterloo.ca/~schepers/formats/TAP.TXT
# - https://wav-prg.sourceforge.io/tape.html
# - https://github.com/francescovannini/truetape64
#
# Dependencies:
# -------------
# - pySerial
#
# Operating Instructions:
# -----------------------
# - Set the selector switch on your TapeBuddy64 to "UART"
# - Connect your TapeBuddy64 to your Commodore Datasette
# - Connect your TapeBuddy64 to a USB port of your PC
# - Execute this skript: python tape-write.py inputfile.tap
# - Press RECORD on your Datasette
# - The writing is done fully automatically. It stops when the file is recorded, 
#   the end of the cassette is reached or when the STOP button on the Datasette 
#   is pressed.


import sys
import os
from serial import Serial
from serial.tools.list_ports import comports


# ===================================================================================
# Adapter Class - Basic Communication with the Device via USB to Serial Converter
# ===================================================================================

class Adapter(Serial):
    def __init__(self):
        super().__init__(baudrate = 460800, timeout = 1, write_timeout = 1)
        self.identify()


    def identify(self):
        vid = '1A86'
        pid = '7523'
        did = 'TapeBuddy64'
        for p in comports():
            if vid and pid in p.hwid:
                self.port = p.device

                try:
                    self.open()
                except:
                    continue

                try:
                    self.sendcommand('i')
                    data = self.getline()
                except:
                    self.close()
                    continue

                if data == did:
                    break
                else:
                    self.close()


    def sendcommand(self, cmd):
        self.write(cmd.encode())


    def getline(self):
        return self.readline().decode().rstrip('\r\n')


    def getversion(self):
        self.sendcommand('v')
        version = self.getline()
        return version


# ===================================================================================
# Progress bar
# ===================================================================================

def progress(percent=0, width=50):
    left = width * percent // 100
    right = width - left
    sys.stdout.write('\r[' + '#' * left + ' ' * right + '] ' + str(percent) + '%')
    sys.stdout.flush()


# ===================================================================================
# Main Function
# ===================================================================================

# Print Header
print('')
print('---------------------------------------------------------')
print('TapeBuddy64 - Python Command Line Interface v1.0')
print('(C) 2021 by Stefan Wagner - github.com/wagiminator')
print('---------------------------------------------------------')

# Get and check command line arguments
try:
    fileName = sys.argv[1]
except:
    sys.stderr.write('ERROR: Missing input file name\n')
    sys.exit(1)


# Establish serial connection
print('Connecting to TapeBuddy64 ...')
tapebuddy = Adapter()

if not tapebuddy.is_open:
    sys.stderr.write('ERROR: Device not found\n')
    sys.exit(1)

print('Device found on port', tapebuddy.port)
print('Firmware version:', tapebuddy.getversion())


# Open input file and check file header
print('Opening', fileName, 'for reading ...')

try:
    fileSize = os.stat(fileName).st_size
except:
    sys.stderr.write ('ERROR: File not found\n')
    sys.exit(1)

try:
    f = open(fileName, 'rb')
except:
    sys.stderr.write('ERROR: Could not open input file\n')
    tapebuddy.close()
    sys.exit(1)

if not f.read(12) == b'C64-TAPE-RAW':
    sys.stderr.write('ERROR: Wrong file header\n')
    tapebuddy.close()
    f.close()
    sys.exit(1)


# Get TAP version and size of data area
tapversion = f.read(1)[0]
f.seek(16)
datasize = int.from_bytes(f.read(4), byteorder='little')

if not (datasize + 20) == fileSize:
    sys.stderr.write('ERROR: File size does not match header entry\n')
    tapebuddy.close()
    f.close()
    sys.exit(1)


# Print TAP file information
print('File header: OK')
print('File size:   OK')
print('TAP version:', tapversion)


# Preparing data and store it in temp file
print('Preparing data ...')

try:
    t = open('tapebuddy.tmp', 'wb')
except:
    sys.stderr.write('ERROR: Could not create temp file\n')
    tapebuddy.close()
    f.close()
    sys.exit(1)

fcount  = datasize
tcount  = 0
taptime = 0

while fcount > 0:
    dataval = f.read(1)[0]
    fcount -= 1
    if dataval > 0:
        #dataval = int(dataval / 0.985248 + 0.5)
        t.write(dataval.to_bytes(2, byteorder='little'))
        tcount  += 2
        taptime += dataval
    else: 
        if tapversion == 1:
            dataval  = int.from_bytes(f.read(3), byteorder='little') // 8
            #dataval  = int(dataval / 0.985248 + 0.5)
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


# Send write command to TapeBuddy64 and wait for RECORD pressed
print('PRESS RECORD ON TAPE')
tapebuddy.sendcommand('w')

while 1:
    response = tapebuddy.read(1)
    if response:
        break
    sys.stdout.write('#')
    sys.stdout.flush()
print('\r', ' ' * 50, end='\r')
if response[0] > 0:
    f.close()
    tapebuddy.close()
    sys.stderr.write('TIMEOUT: Didn\'t I say something about pressing RECORD?\n')
    sys.exit(1)
else:
    print('OK')
    print('Start recording ...')


# Read data from temp file and write to tape
t = open('tapebuddy.tmp', 'rb')
checksum = 0

while 1:
    response = tapebuddy.read(1)
    if response:
        packsize = response[0]
        if packsize == 0:
            break
        if tcount <= 0:
            tapebuddy.write(b'\x00\x00')
        while packsize > 0 and tcount > 0:
            data = t.read(2)
            tcount -= 2
            tapebuddy.write(data)
            packsize -= 2
            checksum += data[0]
            checksum += data[1]
            if packsize > 0 and tcount <= 0:
                tapebuddy.write(b'\x00\x00')
            checksum %= 65536
        progress((datasize - tcount) * 100 // datasize)

print('')
testsum  = int.from_bytes(tapebuddy.read(2), byteorder='little')
underrun = tapebuddy.read(1)[0]
stopped  = tapebuddy.read(1)[0]


# Close temp file and serial connection
t.close()
tapebuddy.close()


# Validate data and checksum and print infos
if underrun > 0:
    sys.stderr.write('ERROR: Buffer underrun occured\n')

if tcount > 0 or stopped > 0:
    sys.stderr.write('ERROR: Recording was stopped before completion\n')
    sys.exit(1)

print('Recording finished.')

if not checksum == testsum:
    sys.stderr.write('ERROR: Checksum mismatch\n')
    sys.exit(1)

print('Checksum:          OK')
print('Buffer underruns:  No')
print('Recording successful!')
print('PRESS STOP ON TAPE');
print('')


# Delete temp file and exit
try:
    os.remove('tapebuddy.tmp')
except:
    sys.stderr.write('ERROR: Could not delete temp file\n')

sys.exit(0)
