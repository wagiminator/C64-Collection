#!/usr/bin/env python3
# ===================================================================================
# Project:   DumpMaster64 - Python Script - Detect IEC Devices
# Version:   v1.3
# Year:      2022
# Author:    Stefan Wagner
# Github:    https://github.com/wagiminator
# License:   http://creativecommons.org/licenses/by-sa/3.0/
# ===================================================================================
#
# Description:
# ------------
# Detects devices on IEC bus and prints their status
#
# Dependencies:
# -------------
# - adapter (included in libs folder)
#
# Operating Instructions:
# -----------------------
# - Set the serial mode switch on your DumpMaster64 adapter to "UART"
# - Connect the adapter to your floppy disk drive(s)
# - Connect the adapter to a USB port of your PC
# - Switch on your floppy disk drive(s)
# - Execute this skript: python disk-detect.py


import sys
from libs.adapter import *


# Print Header
print('')
print('--------------------------------------------------')
print('DumpMaster64 - Python Command Line Interface v1.3')
print('(C) 2022 by Stefan Wagner - github.com/wagiminator')
print('--------------------------------------------------')


# Establish serial connection
print('Connecting to DumpMaster64 ...')
dumpmaster = Adapter()
if not dumpmaster.is_open:
    raise AdpError('Adapter not found')
print('Adapter found on port', dumpmaster.port)
print('Firmware version:', dumpmaster.getversion())
print('')


# Detect IEC devices
print('Detecting IEC devices ...')
for device in range(8, 12):
    print((str(device) + ':').ljust(4), end='')
    magic = dumpmaster.detectdevice(device)
    if not magic:
        print('No response')
    elif magic in IEC_DEVICES:
        status = dumpmaster.getstatus()
        if not status: status = 'unknown'
        print(IEC_DEVICES[magic], ', status:', status)
    else:
        print('Unknown device')
dumpmaster.close()
print('')
