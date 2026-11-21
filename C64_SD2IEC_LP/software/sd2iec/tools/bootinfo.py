#!/usr/bin/env python3
# ===================================================================================
# Add the boot information for the newboot bootloader to a firmware binary
# ===================================================================================
# Usage: bootinfo.py <file.bin> <application size> <config.h>
#
# The newboot bootloader flashes a *.bin file from the root of the SD card if the
# size, the device ID and the CRC are valid. The file is padded to the application
# size; the last 8 bytes hold the device ID (4), the version (2) and the CRC-CCITT
# of the file (2). With version 0 (used here) the file is flashed whenever its CRC
# differs from the installed firmware, so a file left on the card is not flashed
# again. The device ID is read from BOOT_DEVID in config.h.
#
# 2026 by Stefan Wagner:   https://github.com/wagiminator
import re, struct, sys

def crc_ccitt(data):
    crc = 0xFFFF
    for b in data:
        b ^= crc & 0xFF
        b = (b ^ (b << 4)) & 0xFF
        crc = (((b << 8) & 0xFFFF) | (crc >> 8)) ^ (b >> 4) ^ ((b << 3) & 0xFFFF)
    return crc

name, size = sys.argv[1], int(sys.argv[2], 0)
config = open(sys.argv[3]).read()
devid = int(re.search(r'#define\s+BOOT_DEVID\s+(0x[0-9A-Fa-f]+)', config).group(1), 16)
data = bytearray(open(name, 'rb').read())
if len(data) > size - 8:
    sys.exit('firmware too large: %d bytes, %d available' % (len(data), size - 8))
data += b'\xFF' * (size - len(data))
data[size - 8:size - 2] = struct.pack('<IH', devid, 0)
data[size - 2:] = struct.pack('<H', crc_ccitt(data[:size - 2]))
open(name, 'wb').write(data)
