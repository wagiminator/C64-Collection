#!/usr/bin/env python3
# ===================================================================================
# Project:   DiskBuddy64 - Python Script - Adapter Library
# Version:   v1.0
# Year:      2022
# Author:    Stefan Wagner
# Github:    https://github.com/wagiminator
# License:   http://creativecommons.org/licenses/by-sa/3.0/
# ===================================================================================
#
# Description:
# ------------
# Adapter Library - Basic functions for the TapeBuddy64
#
# Reference:
# ----------
# - PETSCII/ASCII table from https://github.com/AndiB/PETSCIItoASCII
#
# Dependencies:
# -------------
# - pySerial


import os
import sys
from serial import Serial
from serial.tools.list_ports import comports


# ===================================================================================
# Adapter Class - Basic Communication with the Device via USB to Serial Converter
# ===================================================================================


class Adapter(Serial):
    def __init__(self):
        super().__init__(baudrate = 460800, timeout = 1, write_timeout = 1)
        self.identify()


    # Identify the com port of the adapter
    def identify(self):
        vid = '1A86'
        pid = '7523'
        did = 'DiskBuddy64'
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


    # Send a command to the adapter
    def sendcommand(self, cmd, argbytes = None):
        self.write(cmd.encode())
        if argbytes:
            self.write(len(argbytes).to_bytes(1, byteorder='little'))
            self.write(argbytes)
        else:
            self.write(b'\x00')


    # Get a reply string from the adapter
    def getline(self):
        return self.readline().decode().rstrip('\r\n')


    # Get firmware version of the adapter
    def getversion(self):
        self.sendcommand('v')
        version = self.getline()
        return version


    # Upload binary to disk drive RAM
    def uploadbin(self, ramaddr, filename):
        try:
            filesize = os.stat(filename).st_size
        except:
            sys.stderr.write('ERROR: ' + filename + ' not found\n')
            return 1

        try:
            f = open(filename, 'rb')
        except:
            sys.stderr.write('ERROR: Could not open ' + filename + '\n')
            return 1

        while filesize > 0:
            if filesize > 32:   length = 32
            else:               length = filesize
            argstr  = b'M-W'
            argstr += ramaddr.to_bytes(2, byteorder='little')
            argstr += length.to_bytes(1, byteorder='little')
            argstr += f.read(length)
            self.sendcommand('c', argstr)
            reply = self.read(1)
            if not reply or reply[0] > 0:
                f.close()
                sys.stderr.write('ERROR: Uploading ' + filename + ' failed\n')
                return 1
            ramaddr  += length
            filesize -= length
        f.close()
        return 0


    # Read single block from disk
    def readblock(self, ramaddr, track, sector):
        seclist = sector.to_bytes(1, byteorder='little')
        if self.startfastiec('r', ramaddr, track, seclist) > 0:
            return None
        self.timeout = 4
        block = self.read(256)
        self.timeout = 1
        if not block or not len(block) == 256:
            sys.stderr.write('ERROR: Reading from the disk failed\n')
            return None
        return block


    # Start reading/writing list of sectors from/to a single track
    def startfastiec(self, cmd, ramaddr, track, seclist):
        argstr  = b'M-E'
        argstr += ramaddr.to_bytes(2, byteorder='little')
        argstr += track.to_bytes(1, byteorder='little')
        argstr += len(seclist).to_bytes(1, byteorder='little')
        for x in seclist: argstr += x.to_bytes(1, byteorder='little')
        self.sendcommand(cmd, argstr)
        reply = self.read(1)
        if not reply or reply[0] > 0:
            sys.stderr.write('ERROR: Sending command failed\n')
            return 1
        return 0


    # Start formating disk
    def startformat(self, ramaddr, tracks, bump, demag, diskname, diskident):
        argstr  = b'M-E'
        argstr += (ramaddr + 3).to_bytes(2, byteorder='little')
        argstr += (tracks + 1).to_bytes(1, byteorder='little')
        argstr += b'\x01'
        argstr += bump.to_bytes(1, byteorder='little')
        argstr += b'\x01'
        argstr += demag.to_bytes(1, byteorder='little')
        argstr += b'\x000:'
        argstr += (diskname + ',' + diskident).encode()
        self.sendcommand('f', argstr)
        reply = self.read(1)
        if not reply or reply[0] > 0:
            sys.stderr.write('ERROR: Sending command failed\n')
            return 1
        return 0


    # Set IEC device number
    def setdevice(self, device):
        argstr = device.to_bytes(1, byteorder='little')
        self.sendcommand('\x0E', argstr)
        reply = self.read(1)
        if not reply or reply[0] > 0:
            return 1
        return 0


    # Check if IEC device is present
    def checkdevice(self, device):
        if self.setdevice(device) > 0:
            return None
        self.sendcommand('s')
        response = self.getline()
        return response


# ===================================================================================
# BAM Class - Working with the BAM
# ===================================================================================


class BAM:
    def __init__(self, bam):
        self.bam = bam

    def getdiskname(self):
        return PETtoASC(PETdelpadding(self.bam[0x90:0xA0]))

    def getdiskident(self):
        return PETtoASC(self.bam[0xA2:0xA4])

    def getheader(self):
        header  = '0    \"'
        header += (self.getdiskname() + '\"').ljust(19)
        header += self.getdiskident().ljust(3)
        header += PETtoASC(self.bam[0xA5:0xA7])
        return header.upper()

    def getblocksfree(self):
        blocksfree = 0
        for x in range(0x04, 0x90, 0x04): 
            if not x == 0x48: blocksfree += self.bam[x]
        return blocksfree

    def getallocated(self):
        allocated = 0
        for x in range(0x04, 0x90, 0x04): 
            allocated += (getsectors(x // 4) - self.bam[x])
        return allocated

    def blockisfree(self, track, sector):
        return self.bam[4 * track + 1 + (sector // 8)] & (1 << (sector % 8)) > 0


# ===================================================================================
# Error Class - Raise an Error
# ===================================================================================


class AdpError(Exception):
    def __init__(self, msg='Something went wrong'):
        super(AdpError, self).__init__(msg)
        sys.stderr.write('ERROR: ' + msg + '\n')
        sys.exit(1)
        

# ===================================================================================
# Basic Functions for the TapeBuddy64
# ===================================================================================


# Get number of sectors in track
def getsectors(track):
    if track <  1:  return 0
    if track < 18:  return 21
    if track < 25:  return 19
    if track < 31:  return 18
    if track < 41:  return 17
    return 0

# Get pointer to track/sector in D64 file
def getfilepointer(track, sector):
    if track == 1: return sector * 256
    pointer = 0
    for x in range(1, track):
        pointer += 256 * getsectors(x)
    return pointer + 256 * sector


# Convert PETSCII bytes to ASCII string
def PETtoASC(line):
    result = ''
    for x in line:
        result += chr(PETtoASCtable[x])
    return result


# Convert ASCII string to PETSCII string
def ASCtoPET(line):
    result = ''
    for x in line:
        result += chr(ASCtoPETtable[ord(x)])
    return result

# Delete $A0 padding in PETSCII bytes
def PETdelpadding(line):
    result = b''
    for x in line:
        if not x == 0xA0:
              result += x.to_bytes(1, byteorder='little')
    return result


# ===================================================================================
# PETSCII to ASCII Conversion Tables - from https://github.com/AndiB/PETSCIItoASCII
# ===================================================================================


# PETSCII to ASCII conversion table
PETtoASCtable = [
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x14,0x09,0x0d,0x11,0x93,0x0a,0x0e,0x0f,
    0x10,0x0b,0x12,0x13,0x08,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
    0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
    0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
    0x40,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
    0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x5b,0x5c,0x5d,0x5e,0x5f,
    0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
    0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
    0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
    0x90,0x91,0x92,0x0c,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
    0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
    0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
    0x60,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
    0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x7b,0x7c,0x7d,0x7e,0x7f,
    0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
    0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf]


# ASCII to PETSCII conversion table
ASCtoPETtable = [
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x14,0x20,0x0d,0x11,0x93,0x0a,0x0e,0x0f,
    0x10,0x0b,0x12,0x13,0x08,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
    0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
    0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
    0x40,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
    0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0x5b,0x5c,0x5d,0x5e,0x5f,
    0xc0,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
    0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0xdb,0xdc,0xdd,0xde,0xdf,
    0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
    0x90,0x91,0x92,0x0c,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
    0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
    0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
    0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
    0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
    0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
    0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff]
