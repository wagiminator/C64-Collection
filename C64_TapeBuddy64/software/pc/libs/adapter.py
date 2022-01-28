#!/usr/bin/env python3
# ===================================================================================
# Project:   TapeBuddy64 - Python Script - Adapter Library
# Version:   v1.1
# Year:      2021
# Author:    Stefan Wagner
# Github:    https://github.com/wagiminator
# License:   http://creativecommons.org/licenses/by-sa/3.0/
# ===================================================================================
#
# Description:
# ------------
# Adapter Library - Basic functions for the TapeBuddy64
#
# Dependencies:
# -------------
# - pySerial


import sys
from serial import Serial
from serial.tools.list_ports import comports


# ===================================================================================
# Adapter Class - Basic Communication with the Device via USB to Serial Converter
# ===================================================================================

class Adapter(Serial):
    def __init__(self, ident='TapeBuddy64'):
        super().__init__(baudrate = 460800, timeout = 1, write_timeout = 1)
        self.identify(ident)

    # Identify the com port of the adapter
    def identify(self, ident):
        vid = '1A86'
        pid = '7523'
        for p in comports():
            if vid and pid in p.hwid:
                self.port = p.device

                try:
                    self.open()
                except:
                    continue

                try:
                    self.sendcommand(CMD_GETIDENT)
                    data = self.getline()
                except:
                    self.close()
                    continue

                if data == ident:
                    break
                else:
                    self.close()

    # Send a command to the adapter
    def sendcommand(self, cmd):
        self.write(cmd.encode())

    # Get a reply string from the adapter
    def getline(self):
        return self.readline().decode().rstrip('\r\n')

    # Get firmware version of the adapter
    def getversion(self):
        self.sendcommand(CMD_GETVERSION)
        version = self.getline()
        return version


# ===================================================================================
# Error Class - Raise an Error
# ===================================================================================

class AdpError(Exception):
    def __init__(self, msg='Something went wrong'):
        super(AdpError, self).__init__(msg)
        sys.stderr.write('ERROR: ' + msg + '\n\n')
        sys.exit(1)


# ===================================================================================
# Adapter Commands
# ===================================================================================

CMD_GETIDENT   = 'i'
CMD_GETVERSION = 'v'
CMD_READTAPE   = 'r'
CMD_WRITETAPE  = 'w'
