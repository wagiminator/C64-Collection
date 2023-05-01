#!/usr/bin/env python3
# ===================================================================================
# Project:   DiskMaster64 - Python Script - Detect IEC Devices
# Version:   v1.0
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
# - Connect the adapter to your floppy disk drive(s)
# - Connect the adapter to a USB port of your PC
# - Switch on your floppy disk drive(s)
# - Execute this skript: python disk-detect.py


import sys
from libs.adapter import *


# Print Header
print('')
print('--------------------------------------------------')
print('DiskMaster64 - Python Command Line Interface v1.0')
print('(C) 2022 by Stefan Wagner - github.com/wagiminator')
print('--------------------------------------------------')


# Establish serial connection
print('Connecting to DiskMaster64 ...')
diskmaster = Adapter()
if not diskmaster.is_open:
    raise AdpError('Adapter not found')
print('Adapter found on port', diskmaster.port)
print('Firmware version:', diskmaster.getversion())
print('')


# Detect IEC devices
print('Detecting IEC devices ...')
for device in range(8, 12):
    print((str(device) + ':').ljust(4), end='')
    magic = diskmaster.detectdevice(device)
    if not magic:
        print('No response')
    elif magic in IEC_DEVICES:
        status = diskmaster.getstatus()
        if not status: status = 'unknown'
        print(IEC_DEVICES[magic], ', status:', status)
    else:
        print('Unknown device')
diskmaster.close()
print('')
