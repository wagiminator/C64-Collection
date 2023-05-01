#!/usr/bin/env python3
# ===================================================================================
# Project:   DiskMaster64 - Python Script - Disk Status
# Version:   v1.0
# Year:      2022
# Author:    Stefan Wagner
# Github:    https://github.com/wagiminator
# License:   http://creativecommons.org/licenses/by-sa/3.0/
# ===================================================================================
#
# Description:
# ------------
# Print status of floppy disk drives
#
# Dependencies:
# -------------
# - adapter (included in libs folder)
#
# Operating Instructions:
# -----------------------
# - python disk-status.py


import sys
from libs.adapter import *


# Establish serial connection
print('Connecting to DiskMaster64 ...')
diskmaster = Adapter()

if not diskmaster.is_open:
    raise AdpError('Adapter not found')

print('Adapter found on port', diskmaster.port)
print('Firmware version:', diskmaster.getversion())
print('Reading status ...')
for device in range(8, 12):
    diskmaster.setdevice(device)
    response = diskmaster.getstatus()
    if response: print((str(device) + ':').ljust(4) + response)
    else:        print((str(device) + ':').ljust(4) + 'No response')
diskmaster.close()
