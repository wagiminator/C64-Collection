#!/usr/bin/env python3
# ===================================================================================
# Project:   DumpMaster64 - Python Script - Adapter Library
# Version:   v1.3
# Year:      2022
# Author:    Stefan Wagner
# Github:    https://github.com/wagiminator
# License:   http://creativecommons.org/licenses/by-sa/3.0/
# ===================================================================================
#
# Description:
# ------------
# Adapter Library - Basic functions for the DumpMaster64
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
    def __init__(self, ident='DumpMaster64'):
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
        return self.readline().decode().rstrip('\r\n')

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

    # Reset IEC devices
    def reset(self):
        return self.iec_command(CMD_RESET)

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

    # Initialize disk
    def initialize(self):
        return self.iec_command(CMD_IEC_CMD, b'I')

    # Read from memory of IEC device
    def readmemory(self, addr, size):
        data = bytes()
        while size > 0:
            if size > 256:   length = 256
            else:            length = size
            ieccmd  = b'M-R'
            ieccmd += addr.to_bytes(2, byteorder='little')
            ieccmd += bytes([length % 256])
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

    # Execute subroutine in memory
    def executememory(self, addr):
        ieccmd  = b'M-E'
        ieccmd += addr.to_bytes(2, byteorder='little')
        return self.iec_command(CMD_IEC_CMD, ieccmd)


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

    # Upload binary to disk drive RAM via fast IEC
    def fastuploadbin(self, loadaddr, filename):
        try:
            f = open(filename, 'rb')
        except:
            sys.stderr.write('ERROR: Failed to open ' + filename + '\n')
            return 1

        reply = self.sendmemory(loadaddr, f.read())
        f.close()
        return reply

    # Read single sector from disk and return block data
    def readblock(self, track, sector):
        seclist = bytes([sector])
        for retry in range(3):
            if self.startfastread(track, seclist) > 0:
                return None
            self.timeout = 4
            block = self.getblockgcr()
            self.timeout = 1
            if block and len(block) == 256:
                return block
        return None

    # Get raw GCR-encoded block via fast IEC and decode
    def getblockgcr(self):
        return decodeblock(self.getblock(325))

    # Get block data via fast IEC
    def getblock(self, size):
        reply = self.read(1)
        if not reply or reply[0] > 0:
            return None
        block = self.read(size)
        if not block or not len(block) == size:
            return None
        return block

    # GCR-encode data block and send via fast IEC
    def sendblockgcr(self, data):
        return self.sendblock(encodeblock(data))

    # Send block data via fast IEC
    def sendblock(self, data):
        reply = self.read(1)
        if not reply or reply[0] > 0:
            return 1
        if not self.write(data) == len(data):
            return 1
        return 0

    # Write to memory of IEC device via fast IEC
    def sendmemory(self, addr, data):
        size = len(data)
        while size > 0:
            if size > 256:  length = 256
            else:           length = size
            ieccmd  = b'M-E'
            ieccmd += FASTUPLOAD_STARTADDR.to_bytes(2, byteorder='little')
            ieccmd += addr.to_bytes(2, byteorder='little')
            ieccmd += bytes([length % 256])
            if self.iec_command(CMD_IEC_CMD, ieccmd) > 0: return 1
            if self.iec_command(CMD_WRITEFAST, bytes([length % 256])) > 0: return 1
            self.write(data[:length])
            reply = self.read(1)
            if not reply or reply[0] > 0: return 1
            data  = data[length:]
            addr += length
            size -= length
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

    # Start reading a file from disk
    def startfastload(self, track, sector):
        ieccmd  = b'M-E'
        ieccmd += FASTLOAD_STARTADDR.to_bytes(2, byteorder='little')
        ieccmd += bytes([track, sector])
        return self.iec_command(CMD_LOADFILE, ieccmd)

    # Start formating disk using fastformat
    def startfastformat(self, tracks, bump, demag, verify, diskname, diskident):
        ieccmd  = b'M-E'
        ieccmd += FASTFORMAT_STARTADDR.to_bytes(2, byteorder='little')
        ieccmd += bytes([tracks, bump, demag, verify])
        ieccmd += (':' + diskname + ',' + diskident).encode()
        return self.iec_command(CMD_FORMATDISK, ieccmd)


# ===================================================================================
# GCR Encoding and Decoding Functions
# ===================================================================================

# Encode an entire block
def encodeblock(data):
    if not data or not len(data) == 256:
        return None
    parity = 0
    for x in data: parity ^= x
    data = b'\x07' + data + bytes([parity, 0x00, 0x00])
    return encodedata(data)

# Encode data stream
def encodedata(data):
    result = bytes()
    for q in range(0, len(data), 4): result += encodequartet(data[q:q+4])
    return result

# Encode 4 bytes
def encodequartet(data):
    temp  = 0
    for i in range(4):
        temp <<= 5
        temp  += GCR_TABLE[data[i] >> 4]
        temp <<= 5
        temp  += GCR_TABLE[data[i] & 0x0F]
    result = bytes()
    for i in range(5):
        result += bytes([(temp >> (8 * (4 - i))) & 0xFF])
    return result

# Decode an entire block
def decodeblock(data):
    if not data or not len(data) == 325:
        return None
    data = decodedata(data)
    parity = data[257];
    data = data[1:257]
    for x in data: parity ^= x
    if not parity == 0:
        return b'\x01'
    return data

# Decode data stream
def decodedata(data):
    result = bytes()
    for q in range(0, len(data), 5): result += decodequintet(data[q:q+5])
    return result

# Decode 5 bytes
def decodequintet(data):
    temp  = 0
    for i in range(5):
        temp <<= 8
        temp  += (data[i])
    result = bytes()
    for i in range(4):
        tempbyte = (temp >> (10 * (3 - i))) & 0x3FF
        result  += bytes([(GCR_DECTAB[tempbyte >> 5] << 4) + GCR_DECTAB[tempbyte & 0x1F]])
    return result

# GCR encoding table
GCR_TABLE = [0b01010, 0b01011, 0b10010, 0b10011, 0b01110, 0b01111, 0b10110, 0b10111,
             0b01001, 0b11001, 0b11010, 0b11011, 0b01101, 0b11101, 0b11110, 0b10101]

# GCR decoding table
GCR_DECTAB = [0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 1, 0, 12, 4, 5, 
              0, 0, 2, 3, 0, 15, 6, 7, 0, 9, 10, 11, 0, 13, 14, 0]


# ===================================================================================
# Error Class - Raise an Error
# ===================================================================================

class AdpError(Exception):
    def __init__(self, msg='Something went wrong'):
        super(AdpError, self).__init__(msg)
        sys.stderr.write('ERROR: ' + msg + '\n\n')
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
FASTREAD_STARTADDR   = 0x0503
FASTWRITE_LOADADDR   = 0x0500
FASTWRITE_STARTADDR  = 0x0503
FASTLOAD_LOADADDR    = 0x0500
FASTLOAD_STARTADDR   = 0x0503
FASTFORMAT_LOADADDR  = 0x0500
FASTFORMAT_STARTADDR = 0x0503
FASTUPLOAD_LOADADDR  = 0x0400
FASTUPLOAD_STARTADDR = 0x0400

MEMCMD_INITIALIZE    = 0xC63D
MEMCMD_LOADBAM       = 0xD042
MEMCMD_SETTRACK18    = 0xD00E


# ===================================================================================
# Adapter Commands
# ===================================================================================

# High level commands
CMD_GETIDENT   = 'i'
CMD_GETVERSION = 'v'
CMD_READTAPE   = 'R'
CMD_WRITETAPE  = 'W'
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
CMD_SETRST     = '\x1f'
CMD_RELRST     = '\x20'
CMD_GETSENSE   = '\x21'
CMD_MOTORON    = '\x22'
CMD_MOTOROFF   = '\x23'
