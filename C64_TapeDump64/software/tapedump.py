#!/usr/bin/env python3
# ===================================================================================
# Project:   TapeDump64 - Python Script for Command Line Interface
# Version:   v1.1
# Year:      2021
# Author:    Stefan Wagner
# Github:    https://github.com/wagiminator
# License:   http://creativecommons.org/licenses/by-sa/3.0/
# ===================================================================================
#
# Description:
# ------------
# TapeDump64 is a simple and inexpensive adapter that can interface a Commodore
# Datasette to your PC via USB in order to dump your software saved on tapes as
# TAP files.
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
# - Set the serial mode switch on your TapeDump64 to "UART"
# - Connect your TapeDump64 to your Commodore Datasette
# - Connect your TapeDump64 to a USB port of your PC
# - Execute this skript: python tapedump.py outputfile.tap
# - Press PLAY on your Datasette
# - The dumping is done fully automatically. It stops when the end of the cassette
#   is reached, when there are no more signals on the tape for a certain time or
#   when the STOP button on the Datasette is pressed.


import sys
import time
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
        did = 'TapeDump64'
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
# Main Function
# ===================================================================================

# Print Header
print('--------------------------------------------------')
print('TapeDump 64 - Python Command Line Interface v1.1')
print('(C) 2021 by Stefan Wagner - github.com/wagiminator')
print('--------------------------------------------------')

# Get and check command line arguments
try:
    fileName = sys.argv[1]
except:
    sys.stderr.write('ERROR: Missing output file name\n')
    sys.exit(1)


# Establish serial connection
print('Connecting to TapeDump64 ...')
tapedump = Adapter()

if not tapedump.is_open:
    sys.stderr.write('ERROR: Device not found\n')
    sys.exit(1)

print('Device found on port', tapedump.port)
print('Firmware version:', tapedump.getversion())


# Open output file and write file header
print('Opening', fileName, 'for writing ...')

try:
    f = open(fileName, 'wb')
except:
    sys.stderr.write('ERROR: Could not open output file\n')
    tapedump.close()
    sys.exit(1)

f.write(b'C64-TAPE-RAW\x00\x00\x00\x00\x00\x00\x00\x00')


# Send read command to TapeDump64 and wait for PLAY pressed
print('PRESS PLAY ON TAPE')
data = None
tapedump.sendcommand('r')
while 1:
    data = tapedump.read(1)
    if data:
        break
    sys.stdout.write('*')
    sys.stdout.flush()
print('\r', ' ' * 50, end='\r')
if data[0] > 0:
    f.close()
    tapedump.close()
    sys.stderr.write('TIMEOUT: Didn\'t I say something about pressing PLAY?\n')
    sys.exit(1)
else:
    print('OK')
    print('Searching ...')


# Receive data from TapeDump64 and write to output file
count     = 0
checksum  = 0
minpulse  = 65535
maxpulse  = 0
taptime   = 0
starttime = time.time()
msgtime   = time.time()

while 1:
    data = tapedump.read(1)
    if data:
        if data[0] == 0:
            break
        if data[0] == 255:
            f.write(b'\x00')
        else:
            f.write(data)
            minpulse  = min(minpulse, data[0])
            maxpulse  = max(maxpulse, data[0])
        count    += 1
        checksum += data[0]
        checksum %= 65536
        taptime  += data[0]
        if count == 1:
            print('Loading ...')
    if time.time() - msgtime > 0.5:
        if count > 0:
            print('Pulses:', count, end='\r')
        msgtime = time.time()

duration = round(time.time() - starttime)
taptime  = taptime * 8 // 1000000 + 1

f.seek(16)
f.write(count.to_bytes(4, byteorder="little"))
testsum  = int.from_bytes(tapedump.read(2), byteorder='little')
overflow = tapedump.read(1)[0]
print(' ' * 50, end='\r')


# Close output file and serial connection
f.close()
tapedump.close()


# Validate data and checksum, print infos, exit
if count == 0:
    sys.stderr.write('TIMEOUT: No data received\n')
    sys.exit(1)

print('Dumping finished')
print('--------------------------------------------------')
print('Total pulses:      ', count)
print('Min pulse length:  ', minpulse)
print('Max pulse length:  ', maxpulse)
print('Total TAP time:    ', taptime//60, 'min', taptime%60, 'sec')
print('Transfer duration: ', duration//60, 'min', duration%60, 'sec')

errors = 0
if overflow == 0:
    print('Buffer overflows:  ', 'No')
else:
    sys.stderr.write('ERROR: Buffer overflow occured\n')
    errors = 1
if checksum == testsum:
    print('Checksum:          ', 'OK')
else:
    sys.stderr.write('ERROR: Checksum mismatch\n')
    errors = 1

print('--------------------------------------------------')
sys.exit(errors)
