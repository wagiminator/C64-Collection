#!/usr/bin/env python3
# ===================================================================================
# Project:   DiskBuddy64 - Python Script - Adapter Library
# Version:   v1.1
# Year:      2022
# Author:    Stefan Wagner
# Github:    https://github.com/wagiminator
# License:   http://creativecommons.org/licenses/by-sa/3.0/
# ===================================================================================
#
# Description:
# ------------
# Adapter Library - Basic functions for the DiskBuddy64
#
# References:
# -----------
# - IEC device detection adapted from OpenCBM: https://github.com/OpenCBM/OpenCBM
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
    def __init__(self, ident='DiskBuddy64'):
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
    def sendcommand(self, cmd, argbytes = None):
        self.write(cmd.encode())
        if argbytes:
            self.write([len(argbytes)])
            self.write(argbytes)
        else:
            self.write([0])

    # Get a reply string from the adapter
    def getline(self):
        return self.readline().decode().rstrip('\r\n').lstrip('\r\n')

    # Get firmware version of the adapter
    def getversion(self):
        self.sendcommand(CMD_GETVERSION)
        return self.getline()


    # ------------------------------------------------------------------------------
    # Data Layer for IEC Stack
    # ------------------------------------------------------------------------------

    # Send IEC command
    def iec_command(self, adpcmd, ieccmd=None):
        self.sendcommand(adpcmd, ieccmd)
        reply = self.read(1)
        if not reply or reply[0] > 0:
            return 1
        return 0

    # Set IEC device number
    def setdevice(self, device):
        return self.iec_command(CMD_SETDEVICE, bytes([device]))

    # Set IEC device number und read status string
    def checkdevice(self, device):
        if self.setdevice(device) > 0:
            return None
        return self.getstatus()

    # Detect device (returns "magic number" of identified device)
    def detectdevice(self, device):
        if self.setdevice(device) > 0:
            return None
        memtest = self.readmemory(0xFF40, 2)
        if not memtest or not len(memtest) == 2:
            return None
        magic = int.from_bytes(memtest, byteorder='little')
        if magic == 0xAAAA:
            memtest = self.readmemory(0xFFFE, 2)
            temp = int.from_bytes(memtest, byteorder='little')
            if not temp == 0xFE67:
                magic = temp
        return magic

    # Get status string from device
    def getstatus(self):
        self.sendcommand(CMD_GETSTATUS)
        return self.getline()

    # Read from memory of IEC device
    def readmemory(self, addr, size):
        data = bytes()
        while size > 0:
            if size > 255:   length = 255
            else:            length = size
            ieccmd  = b'M-R'
            ieccmd += addr.to_bytes(2, byteorder='little')
            ieccmd += bytes([length])
            if self.iec_command(CMD_READMEM, ieccmd) > 0:
                return None
            data += self.read(length)
            self.read(1)
            addr += length
            size -= length
        return data

    # Write to memory of IEC device
    def writememory(self, addr, data):
        size = len(data)
        while size > 0:
            if size > 32:   length = 32
            else:           length = size
            ieccmd  = b'M-W'
            ieccmd += addr.to_bytes(2, byteorder='little')
            ieccmd += bytes([length])
            ieccmd += data[:length]
            if self.iec_command(CMD_IEC_CMD, ieccmd) > 0:
                return 1
            data  = data[length:]
            addr += length
            size -= length
        return 0


    # ------------------------------------------------------------------------------
    # Fast IEC Communication
    # ------------------------------------------------------------------------------

    # Upload binary to disk drive RAM
    def uploadbin(self, loadaddr, filename):
        try:
            f = open(filename, 'rb')
        except:
            sys.stderr.write('ERROR: Failed to open ' + filename + '\n')
            return 1

        reply = self.writememory(loadaddr, f.read())
        f.close()
        return reply

    # Read single sector from disk and return block data
    def readblock(self, track, sector):
        seclist = bytes([sector])
        if self.startfastread(track, seclist) > 0:
            return None
        self.timeout = 4
        block = self.getblock()
        self.timeout = 1
        return block

    # Get block data via fast IEC
    def getblock(self):
        block = self.read(256)
        if not block or not len(block) == 256:
            return None
        return block

    # Send block data via fast IEC
    def sendblock(self, data):
        counter = 256;
        while counter > 0:
            requested = self.read(1);
            if not requested:
                return 1
            size = requested[0]
            if not self.write(data[:size]) == size:
                return 1
            data = data[size:]
            counter -= size
        return 0

    # Start reading list of sectors from a single track
    def startfastread(self, track, seclist):
        ieccmd  = b'M-E'
        ieccmd += FASTREAD_STARTADDR.to_bytes(2, byteorder='little')
        ieccmd += bytes([track, len(seclist)])
        ieccmd += bytes(seclist)
        return self.iec_command(CMD_READTRACK, ieccmd)

    # Start writing list of sectors to a single track using fastwrite
    def startfastwrite(self, track, seclist):
        ieccmd  = b'M-E'
        ieccmd += FASTWRITE_STARTADDR.to_bytes(2, byteorder='little')
        ieccmd += bytes([track, len(seclist)])
        ieccmd += bytes(seclist)
        return self.iec_command(CMD_WRITETRACK, ieccmd)

    # Start formating disk using fastformat
    def startfastformat(self, tracks, bump, demag, diskname, diskident):
        ieccmd  = b'M-E'
        ieccmd += FASTFORMAT_STARTADDR.to_bytes(2, byteorder='little')
        ieccmd += bytes([tracks + 1, 0x01, bump, 0x01, demag, 0x00])
        ieccmd += b'0:' + (diskname + ',' + diskident).encode()
        return self.iec_command(CMD_FORMATDISK, ieccmd)


# ===================================================================================
# Error Class - Raise an Error
# ===================================================================================

class AdpError(Exception):
    def __init__(self, msg='Something went wrong'):
        super(AdpError, self).__init__(msg)
        sys.stderr.write('ERROR: ' + msg + '\n')
        sys.exit(1)
        

# ===================================================================================
# IEC Devices
# ===================================================================================

# Check if device is in database
def device_is_known(magic):
    if magic is None:
        return False
    return magic in IEC_DEVICES

# Check if device is supported
def device_is_supported(magic):
    if magic is None:
        return False
    return magic in IEC_DEVICES_SPT

# Device dictionary (adapted from https://github.com/OpenCBM/OpenCBM)
IEC_DEVICES = {
    0xFEB6: 'CBM 2031',
    0xAAAA: 'CBM 1540 or 1541',
    0xF00F: 'CBM 1541-II',
    0xCD18: 'CBM 1541C',
    0x10CA: 'DolphinDOS 1541',
    0x6F10: 'SpeedDOS 1541',
    0x2710: 'ProfDOS 1541',
    0x8085: 'JiffyDOS 1541',
    0xAEEA: '64\'er DOS 1541',
    0x180D: 'Turbo 1541',
    0x094C: 'PrologicDOS 1541',
    0xFED7: 'CBM 1570',
    0x02AC: 'CBM 1571',
    0x01BA: 'CBM 1581',
    0x32F0: 'CBM 3040',
    0xC320: 'CBM 4040',
    0x20F8: 'CBM 4040',
    0xF2E9: 'CBM 8050',
    0xC866: 'CBM 8250',
    0xC611: 'CBM 8250'
}

# List of supported devices (this has to be checked!!!)
IEC_DEVICES_SPT = {
    0xAAAA, 0xF00F, 0xCD18, 0x10CA, 0x6F10, 0x2710, 0x8085, 0xAEEA, 0x180D, 0x094C
}


# ===================================================================================
# Adapter Constants
# ===================================================================================

FASTREAD_LOADADDR    = 0x0500
FASTREAD_STARTADDR   = 0x0500
FASTWRITE_LOADADDR   = 0x0500
FASTWRITE_STARTADDR  = 0x0500
FASTLOAD_LOADADDR    = 0x0500
FASTLOAD_STARTADDR   = 0x0500
FASTSAVE_LOADADDR    = 0x0500
FASTSAVE_STARTADDR   = 0x0500
FASTFORMAT_LOADADDR  = 0x0500
FASTFORMAT_STARTADDR = 0x0503


# ===================================================================================
# Adapter Commands
# ===================================================================================

# High level commands
CMD_GETIDENT   = 'i'
CMD_GETVERSION = 'v'
CMD_READTRACK  = 'r'
CMD_WRITETRACK = 'w'
CMD_LOADFILE   = 'l'
CMD_SAVEFILE   = 's'
CMD_FORMATDISK = 'f'
CMD_READMEM    = 'm'
CMD_IEC_CMD    = 'c'
CMD_GETSTATUS  = 't'

# Low level commands
CMD_NOP        = '\x00'
CMD_LISTEN     = '\x01'
CMD_UNLISTEN   = '\x02'
CMD_TALK       = '\x03'
CMD_UNTALK     = '\x04'
CMD_READBYTE   = '\x05'
CMD_READBYTES  = '\x06'
CMD_READRAW    = '\x07'
CMD_WRITEBYTE  = '\x08'
CMD_WRITELAST  = '\x09'
CMD_WRITEBYTES = '\x0a'
CMD_READFAST   = '\x0b'
CMD_WRITEFAST  = '\x0c'
CMD_OPEN       = '\x0d'
CMD_CLOSE      = '\x0e'
CMD_RESET      = '\x0f'
CMD_RELEASE    = '\x10'
CMD_GETDEVICE  = '\x11'
CMD_SETDEVICE  = '\x12'
CMD_GETEOI     = '\x13'
CMD_SETEOI     = '\x14'
CMD_CLREOI     = '\x15'
CMD_GETATN     = '\x16'
CMD_SETATN     = '\x17'
CMD_RELATN     = '\x18'
CMD_GETCLK     = '\x19'
CMD_SETCLK     = '\x1a'
CMD_RELCLK     = '\x1b'
CMD_GETDATA    = '\x1c'
CMD_SETDATA    = '\x1d'
CMD_RELDATA    = '\x1e'
