#!/usr/bin/env python3
# ===================================================================================
# Project:   DiskBuddy64 - Python Script - Detect IEC Devices
# Version:   v1.5
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
# - Set the serial mode switch on your DiskBuddy64 adapter to "UART"
# - Connect the adapter to your floppy disk drive(s)
# - Connect the adapter to a USB port of your PC
# - Switch on your floppy disk drive(s)
# - Execute this skript: python disk-detect.py


import sys
from libs.adapter import *


# Establish serial connection
print('Connecting to DiskBuddy64 ...')
diskbuddy = Adapter()
if not diskbuddy.is_open:
    raise AdpError('Adapter not found')
print('Adapter found on port', diskbuddy.port)
print('Firmware version:', diskbuddy.getversion())
print('')


# Detect IEC devices
print('Detecting IEC devices ...')
for device in range(8, 12):
    print((str(device) + ':').ljust(4), end='')
    magic = diskbuddy.detectdevice(device)
    if not magic:
        print('No response')
    elif magic in IEC_DEVICES:
        status = diskbuddy.getstatus()
        if not status: status = 'unknown'
        print(IEC_DEVICES[magic] + ', status: ', status)
    else:
        print('Unknown device')
diskbuddy.close()
