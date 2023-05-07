#!/usr/bin/env python3
# ===================================================================================
# Project:   DiskMaster64 - Python Script - Flash Firmware
# Version:   v1.0
# Year:      2022
# Author:    Stefan Wagner
# Github:    https://github.com/wagiminator
# License:   http://creativecommons.org/licenses/by-sa/3.0/
# ===================================================================================
#
# Description:
# ------------
# Flashes firmware for the DiskMaster64 adapter.
#
# Dependencies:
# -------------
# - tinyupdi (included in libs folder)
#
# Operating Instructions:
# -----------------------
# - Connect the adapter to a USB port of your PC while pressing the BOOT button
# - Immediatly execute this skript: python flash-firmware.py


import sys
import time
from libs.adapter import *
from libs.chprog import Programmer


# Binary file
FIRMWARE_BIN = 'libs/diskmaster64.bin'


# Print Header
print('')
print('--------------------------------------------------')
print('DiskMaster64 - Python Command Line Interface v1.0')
print('(C) 2022 by Stefan Wagner - github.com/wagiminator')
print('--------------------------------------------------')

diskmaster = Adapter()
if diskmaster.is_open:
    print('DiskMaster64 found on port', diskmaster.port)
    print('Setting device to bootmode ...')
    diskmaster.sendcommand(CMD_BOOTLOADER)
    diskmaster.close()
    time.sleep(1)

try:
    print('Connecting to bootloader ...')
    isp = Programmer()
    isp.detect()
    print('FOUND:', isp.chipname, 'with bootloader v' + isp.bootloader + '.')
    print('Erasing chip ...')
    isp.erase()
    print('Flashing', FIRMWARE_BIN, 'to', isp.chipname, '...')
    with open(FIRMWARE_BIN, 'rb') as f: data = f.read()
    isp.flash_data(data)
    print('SUCCESS:', len(data), 'bytes written.')
    print('Verifying ...')
    isp.verify_data(data)
    print('SUCCESS:', len(data), 'bytes verified.')
    isp.exit()
    
except Exception as ex:
    if str(ex) != '':
        sys.stderr.write('ERROR: ' + str(ex) + '!\n')
    sys.exit(1)
    
print('DONE.')
sys.exit(0)
