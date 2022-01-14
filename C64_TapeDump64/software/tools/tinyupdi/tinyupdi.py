#!/usr/bin/env python3
# ===================================================================================
# Project:   TinyUPDI - Minimal application-specific UPDI programmer based on pyupdi
# Version:   v1.0
# Year:      2022
# Author:    Stefan Wagner
# Github:    https://github.com/wagiminator
# License:   MIT License
# ===================================================================================
#
# Description:
# ------------
# TinyUPDI is a serial UPDI programmer software that is adapted to a specific
# application and reduced to the functions required for this. It is normally used
# for projects with a tinyAVR and a built-in CH340 USB to serial adapter to
# simplify firmware uploading.
#
# References:
# -----------
# TinyUPDI is essentially based on pyupdi and pymcuprog:
# - https://github.com/mraardvark/pyupdi
# - https://github.com/microchip-pic-avr-tools/pymcuprog
#
# Dependencies:
# -------------
# - pyserial
#
# Restrictions:
# -------------
# - Programmer must be a CH340 USB to serial adapter
# - Target MCU must be a 0, 1, or 2-series tinyAVR with 8, 14, or 20 pins
# - Only flash and fuses can be programmed
# - Firmware file must be a binary (not ELF or HEX!)
#
# Operating Instructions:
# -----------------------
# - python tinyupdi.py [-h] -d DEVICE [-e] [-f FLASH] [-fs [FUSES [FUSES ...]]]
#   -h, --help                show help message and exit
#   -d, --device              target device
#   -e, --erase               perform a chip erase (implied with --flash)
#   -f FLASH, --flash FLASH   BIN file to flash
#   -fs [FUSES [FUSES ...]], --fuses [FUSES [FUSES ...]]
#                             fuses to set (syntax: fuse_nr:0xvalue)
# - Example:
#   python tinyupdi.py -d attiny202 -f firmware.bin -fs 2:0x01 6:0x04 8:0x00


import re
import sys
import time
import argparse
import serial
from serial import Serial
from serial.tools.list_ports import comports


# ===================================================================================
# Main Function
# ===================================================================================

def _main():
    # Parse command line arguments
    parser = argparse.ArgumentParser(description='Minimal command line'
                                     ' interface for UPDI programming')
    parser.add_argument('-d', '--device', choices=get_supported_devices(),
                        required=True, help='target device')
    parser.add_argument("-e", "--erase", action="store_true",
                        help="perform a chip erase (implied with --flash)")
    parser.add_argument('-f', '--flash', help='BIN file to flash')
    parser.add_argument('-fs', '--fuses', action='append', nargs='*',
                        help='fuses to set (syntax: fuse_nr:0xvalue)')
    args = parser.parse_args(sys.argv[1:])

    if not any( (args.fuses, args.flash, args.erase) ):
        print('No action (flash or fuses or erase)')
        sys.exit(0)

    # Establish connection via serial to UPDI device
    print('Connecting to device ...')
    tinyupdi = Programmer()
    if not tinyupdi.is_open:
        raise PrgError('Device not found')

    # Enter programming mode
    print('Entering programming mode ...')
    try:
        tinyupdi.enter_progmode()
    except:
        print('Device is locked. Performing unlock with chip erase.')
        tinyupdi.unlock()

    # Get target MCU infos
    print('Pinging target MCU ...')
    device = get_device(tinyupdi.get_device_id())
    if device is None:
        tinyupdi.leave_progmode()
        tinyupdi.close()
        raise PrgError('Unknown or unsupported device')
    print('Found:', device['name'])
    if not args.device == device['name']:
        tinyupdi.leave_progmode()
        tinyupdi.close()
        raise PrgError('Found device is not ' + args.device)
    tinyupdi.flash_size = device['flash_size']
    tinyupdi.flash_pagesize = device['flash_pagesize']

    # Perform chip erase
    if args.erase:
        print('Performing chip erase ...')
        if not tinyupdi.chip_erase():
            tinyupdi.leave_progmode()
            tinyupdi.close()
            raise PrgError('Erasing chip failed')

    # Flash binary file
    if args.flash is not None:
        if not tinyupdi.flash_bin(args.flash):
            tinyupdi.leave_progmode()
            tinyupdi.close()
            raise PrgError('Flashing ' + str(args.flash) + ' failed')

    # Write fuses
    if args.fuses is not None:
        print('Writing and verifying fuses ...')
        for fslist in args.fuses:
            for fsarg in fslist:
                if not re.match("^[0-9]+:0x[0-9a-fA-F]+$", fsarg):
                    tinyupdi.leave_progmode()
                    tinyupdi.close()
                    raise PrgError('Bad fuses format {}. Expected fuse_nr:0xvalue'.format(fsarg))
                lst = fsarg.split(":0x")
                fusenum = int(lst[0])
                value = int(lst[1], 16)
                if not tinyupdi.set_fuse(fusenum, value):
                    tinyupdi.leave_progmode()
                    tinyupdi.close()
                    raise PrgError('Setting fuse' + str(fusenum) + ' to ' + str(value) + ' failed')

    # Finish all up
    tinyupdi.leave_progmode()
    tinyupdi.close()
    print('DONE.')
    sys.exit(0)


# ===================================================================================
# Programmer Class
# ===================================================================================

class Programmer(Serial):
    def __init__(self):
        super().__init__(baudrate = 230400, timeout = 1, parity=serial.PARITY_EVEN, stopbits=serial.STOPBITS_TWO)
        self.syscfg_address = 0x0F00
        self.nvmctrl_address = 0x1000
        self.sigrow_address = 0x1100
        self.fuses_address = 0x1280
        self.userrow_address = 0x1300
        self.flash_pagesize = 64
        self.flash_start = 0x8000
        self.identify()

    # Identify port of programmer
    def identify(self):
        vid = '1A86'
        pid = '7523'
        for p in comports():
            if vid and pid in p.hwid:
                self.port = p.device

                try:
                    self.open()
                except:
                    continue

                self.init()
                if not self.check():
                    self.send_double_break()
                    self.init()
                    if not self.check():
                        self.close()
                        continue

                break


    # ------------------------------------------------------------------------------
    # Pysical Layer for UPDI Stack
    # ------------------------------------------------------------------------------

    # Send a double break to reset the UPDI port
    def send_double_break(self):
        self.close()
        temporary_serial = Serial(self.port, 300, stopbits=serial.STOPBITS_ONE, timeout=1)
        temporary_serial.write([UPDI_BREAK, UPDI_BREAK])
        temporary_serial.read(2)
        temporary_serial.close()
        self.open()

    # Send a char array to UPDI with NO inter-byte delay
    def send(self, command):
        self.write(command)
        self.read(len(command))

    # Receive a frame of a known number of chars from UPDI
    def receive(self, size):
        return self.read(size)


    # ------------------------------------------------------------------------------
    # Datalink Layer for UPDI Stack
    # ------------------------------------------------------------------------------

    # Set the inter-byte delay bit and disable collision detection
    def init(self):
        self.stcs(UPDI_CS_CTRLB, 1 << UPDI_CTRLB_CCDETDIS_BIT)
        self.stcs(UPDI_CS_CTRLA, 1 << UPDI_CTRLA_IBDLY_BIT)

    # Check UPDI by loading CS STATUSA
    def check(self):
        if self.ldcs(UPDI_CS_STATUSA) != 0:
            return True
        return False

    # Load data from Control/Status space
    def ldcs(self, address):
        self.send([UPDI_PHY_SYNC, UPDI_LDCS | (address & 0x0F)])
        response = self.receive(1)
        if len(response) != 1:
            return 0x00
        return response[0]

    # Store a value to Control/Status space
    def stcs(self, address, value):
        self.send([UPDI_PHY_SYNC, UPDI_STCS | (address & 0x0F), value])

    # Load a single byte direct from a 16-bit address
    def ld(self, address):
        self.send([UPDI_PHY_SYNC, UPDI_LDS | UPDI_ADDRESS_16 | UPDI_DATA_8,
                   address & 0xFF, (address >> 8) & 0xFF])
        return self.receive(1)[0]

    # Load a 16-bit word directly from a 16-bit address
    def ld16(self, address):
        self.send([UPDI_PHY_SYNC, UPDI_LDS | UPDI_ADDRESS_16 | UPDI_DATA_16,
                   address & 0xFF, (address >> 8) & 0xFF])
        return self.receive(2)

    # Store a single byte value directly to a 16-bit address
    def st(self, address, value):
        self.send([UPDI_PHY_SYNC, UPDI_STS | UPDI_ADDRESS_16 | UPDI_DATA_8,
                   address & 0xFF, (address >> 8) & 0xFF])
        response = self.receive(1)
        if len(response) != 1 or response[0] != UPDI_PHY_ACK:
            raise Exception('Error with st')
        self.send([value & 0xFF])
        response = self.receive(1)
        if len(response) != 1 or response[0] != UPDI_PHY_ACK:
            raise Exception('Error with st')

    # Store a 16-bit word value directly to a 16-bit address
    def st16(self, address, value):
        self.send([UPDI_PHY_SYNC, UPDI_STS | UPDI_ADDRESS_16 | UPDI_DATA_16,
                   address & 0xFF, (address >> 8) & 0xFF])
        response = self.receive(1)
        if len(response) != 1 or response[0] != UPDI_PHY_ACK:
            raise Exception('Error with st')
        self.send([value & 0xFF, (value >> 8) & 0xFF])
        response = self.receive(1)
        if len(response) != 1 or response[0] != UPDI_PHY_ACK:
            raise Exception('Error with st')

    # Load a number of bytes from the pointer location with pointer post-increment
    def ld_ptr_inc(self, size):
        self.send([UPDI_PHY_SYNC, UPDI_LD | UPDI_PTR_INC | UPDI_DATA_8])
        return self.receive(size)

    # Load a 16-bit word value from the pointer location with pointer post-increment
    def ld_ptr_inc16(self, words):
        self.send([UPDI_PHY_SYNC, UPDI_LD | UPDI_PTR_INC | UPDI_DATA_16])
        return self.receive(words << 1)

    # Set the pointer location
    def st_ptr(self, address):
        self.send([UPDI_PHY_SYNC, UPDI_ST | UPDI_PTR_ADDRESS | UPDI_DATA_16,
                   address & 0xFF, (address >> 8) & 0xFF])
        response = self.receive(1)
        if len(response) != 1 or response[0] != UPDI_PHY_ACK:
            raise Exception('Error with st_ptr')

    # Store data to the pointer location with pointer post-increment
    def st_ptr_inc(self, data):
        self.send([UPDI_PHY_SYNC, UPDI_ST | UPDI_PTR_INC | UPDI_DATA_8, data[0]])
        response = self.receive(1)
        if len(response) != 1 or response[0] != UPDI_PHY_ACK:
            raise Exception('ACK error with st_ptr_inc')
        n = 1
        while n < len(data):
            self.send([data[n]])
            response = self.receive(1)
            if len(response) != 1 or response[0] != UPDI_PHY_ACK:
                raise Exception('Error with st_ptr_inc')
            n += 1

    # Store a 16-bit word value to the pointer location with pointer post-increment
    def st_ptr_inc16(self, data):
        ctrla_ackon = 1 << UPDI_CTRLA_IBDLY_BIT
        ctrla_ackoff = ctrla_ackon | (1 << UPDI_CTRLA_RSD_BIT)
        self.stcs(UPDI_CS_CTRLA, ctrla_ackoff)
        self.send([UPDI_PHY_SYNC, UPDI_ST | UPDI_PTR_INC | UPDI_DATA_16] )
        self.send(data)
        self.stcs(UPDI_CS_CTRLA, ctrla_ackon)

    # Store a value to the repeat counter
    def repeat(self, repeats):
        if (repeats - 1) > UPDI_MAX_REPEAT_SIZE:
            raise Exception('Invalid repeat count')
        repeats -= 1
        self.send([UPDI_PHY_SYNC, UPDI_REPEAT | UPDI_REPEAT_BYTE, repeats & 0xFF])

    # Write a key
    def key(self, size, key):
        if len(key) != 8 << size:
            raise Exception('Invalid KEY length')
        self.send([UPDI_PHY_SYNC, UPDI_KEY | UPDI_KEY_KEY | size])
        self.send(list(reversed(list(key))))


    # ------------------------------------------------------------------------------
    # Application Layer for UPDI Stack
    # ------------------------------------------------------------------------------

    # Get device ID
    def get_device_id(self):
        return int.from_bytes(self.read_data(self.sigrow_address, 3), byteorder='big')

    # Check whether the NVM PROG flag is up
    def in_prog_mode(self):
        if self.ldcs(UPDI_ASI_SYS_STATUS) & (1 << UPDI_ASI_SYS_STATUS_NVMPROG):
            return True
        return False

    # Wait for the device to be unlocked
    def wait_unlocked(self, timeout_ms):
        timeout = Timeout(timeout_ms)
        while not timeout.expired():
            if not self.ldcs(UPDI_ASI_SYS_STATUS) & (1 << UPDI_ASI_SYS_STATUS_LOCKSTATUS):
                return True
        return False

    # Unlock and erase
    def unlock(self):
        self.key(UPDI_KEY_64, UPDI_KEY_CHIPERASE)
        key_status = self.ldcs(UPDI_ASI_KEY_STATUS)
        if not key_status & (1 << UPDI_ASI_KEY_STATUS_CHIPERASE):
            raise Exception('Key not accepted')
        self.reset(apply_reset=True)
        self.reset(apply_reset=False)
        if not self.wait_unlocked(500):
            raise Exception('Failed to chip erase using key')

    # Enter into NVM programming mode
    def enter_progmode(self):
        if self.in_prog_mode():
            return True
        self.key(UPDI_KEY_64, UPDI_KEY_NVM)
        key_status = self.ldcs(UPDI_ASI_KEY_STATUS)
        if not key_status & (1 << UPDI_ASI_KEY_STATUS_NVMPROG):
            raise Exception('Key not accepted')
        self.reset(apply_reset=True)
        self.reset(apply_reset=False)
        if not self.wait_unlocked(100):
            raise Exception('Failed to enter NVM programming mode: device is locked')
        if not self.in_prog_mode():
            raise Exception('Failed to enter NVM programming mode')
        return True

    # Disable UPDI which releases any keys enabled
    def leave_progmode(self):
        self.reset(apply_reset=True)
        self.reset(apply_reset=False)
        self.stcs(UPDI_CS_CTRLB, (1 << UPDI_CTRLB_UPDIDIS_BIT) | (1 << UPDI_CTRLB_CCDETDIS_BIT))

    # Apply or release UPDI reset condition
    def reset(self, apply_reset):
        if apply_reset:
            self.stcs(UPDI_ASI_RESET_REQ, UPDI_RESET_REQ_VALUE)
        else:
            self.stcs(UPDI_ASI_RESET_REQ, 0x00)

    # Wait for the NVM controller to be ready
    def wait_nvm_ready(self):
        timeout = Timeout(10000)
        while not timeout.expired():
            status = self.ld(self.nvmctrl_address + UPDI_NVMCTRL_STATUS)
            if status & (1 << UPDI_NVM_STATUS_WRITE_ERROR):
                return False
            if not status & ((1 << UPDI_NVM_STATUS_EEPROM_BUSY) | (1 << UPDI_NVM_STATUS_FLASH_BUSY)):
                return True
        return False

    # Execute an NVM COMMAND on the NVM CTRL
    def execute_nvm_command(self, command):
        return self.st(self.nvmctrl_address + UPDI_NVMCTRL_CTRLA, command)

    # Does a chip erase using the NVM controller (v0)
    def chip_erase(self):
        if not self.wait_nvm_ready():
            raise Exception('Timeout waiting for flash ready before erase')
        self.execute_nvm_command(UPDI_V0_NVMCTRL_CTRLA_CHIP_ERASE)
        if not self.wait_nvm_ready():
            raise Exception('Timeout waiting for flash ready after erase')
        return True

    # Write a number of words to memory
    def write_data_words(self, address, data):
        if len(data) == 2:
            value = data[0] + (data[1] << 8)
            return self.st16(address, value)
        if len(data) > UPDI_MAX_REPEAT_SIZE << 1:
            raise Exception('Invalid length')
        self.st_ptr(address)
        self.repeat(len(data) >> 1)
        return self.st_ptr_inc16(data)

    # Writes a number of bytes to memory
    def write_data(self, address, data):
        if len(data) == 1:
            return self.st(address, data[0])
        elif len(data) == 2:
            self.st(address, data[0])
            return self.st(address + 1, data[1])
        if len(data) > UPDI_MAX_REPEAT_SIZE:
            raise Exception('Invalid length')
        self.st_ptr(address)
        self.repeat(len(data))
        return self.st_ptr_inc(data)

    # Write a page of data to NVM (v0)
    def write_nvm(self, address, data, use_word_access=True):
        if not self.wait_nvm_ready():
            raise Exception('Timeout waiting for flash ready before page buffer clear')
        self.execute_nvm_command(UPDI_V0_NVMCTRL_CTRLA_PAGE_BUFFER_CLR)
        if not self.wait_nvm_ready():
            raise Exception('Timeout waiting for flash ready after page buffer clear')
        if use_word_access:
            self.write_data_words(address, data)
        else:
            self.write_data(address, data)
        self.execute_nvm_command(UPDI_V0_NVMCTRL_CTRLA_WRITE_PAGE)
        if not self.wait_nvm_ready():
            raise Exception('Timeout waiting for flash ready after page write')

    # Write one fuse value (v0)
    def write_fuse(self, fusenum, value):
        if not self.wait_nvm_ready():
            raise Exception('Timeout waiting for NVM controller to be ready before fuse write')
        address = self.fuses_address + fusenum
        self.st(self.nvmctrl_address + UPDI_NVMCTRL_ADDRL, address & 0xFF)
        self.st(self.nvmctrl_address + UPDI_NVMCTRL_ADDRH, (address >> 8) & 0xFF)
        self.st(self.nvmctrl_address + UPDI_NVMCTRL_DATAL, value & 0xFF)
        self.execute_nvm_command(UPDI_V0_NVMCTRL_CTRLA_WRITE_FUSE)
        if not self.wait_nvm_ready():
            raise Exception('Timeout waiting for NVM controller to be ready after fuse write')

    # Read a number of bytes of data from UPDI
    def read_data(self, address, size):
        if size > UPDI_MAX_REPEAT_SIZE:
            raise Exception('Can\'t read that many bytes in one go')
        self.st_ptr(address)
        if size > 1:
            self.repeat(size)
        return self.ld_ptr_inc(size)

    # Read a number of words of data from UPDI
    def read_data_words(self, address, words):
        if words > UPDI_MAX_REPEAT_SIZE:
            raise Exception('Can\'t read that many words in one go')
        self.st_ptr(address)
        if words > 1:
            self.repeat(words)
        return self.ld_ptr_inc16(words)


    # ------------------------------------------------------------------------------
    # NVM programming algorithm
    # ------------------------------------------------------------------------------

    # Read from flash
    def read_flash(self, address, size):
        pages = size // self.flash_pagesize
        if size % self.flash_pagesize:
            pages += 1
        data = bytes()
        for _ in range(pages):
            data += (self.read_data_words(address, self.flash_pagesize >> 1))
            address += self.flash_pagesize
        return data

    # Write to flash
    def write_flash(self, address, data):
        data = self.pad_data(data, self.flash_pagesize)
        pages = self.page_data(data, self.flash_pagesize)
        for page in pages:
            self.write_nvm(address, page, use_word_access=True)
            address += len(page)

    # Read one fuse value
    def read_fuse(self, fusenum):
        address = self.fuses_address + fusenum
        data = self.ld(address)
        return data

    # Pad data so that there are full pages
    def pad_data(self, data, blocksize):
        return data + b'\x00' * (len(data) % blocksize)

    # Divide data into pages
    def page_data(self, data, size):
        total_length = len(data)
        result = list()
        while len(result) < total_length / size:
            result.append(data[:size])
            data = data[size:]
        return result

    # Flash bin file
    def flash_bin(self, filename):
        print('Opening', filename, '...')
        try:
            f = open(filename, 'rb')
        except:
            return false
        data = f.read()
        f.close()
        print('Performing chip erase ...')
        self.chip_erase()
        print('Flashing firmware ...')
        self.write_flash(self.flash_start, data)
        print('Verifying firmware ...')
        readback = self.read_flash(self.flash_start, len(data))
        return data == readback[:len(data)]

    # Set single fuse
    def set_fuse(self, fusenum, value):
        self.write_fuse(fusenum, value)
        actual_val = self.read_fuse(fusenum)
        return actual_val == value


# ===================================================================================
# Timeout Class
# ===================================================================================

# Simple timeout helper in milliseconds.
class Timeout:
    def __init__(self, timeout_ms):
        self.timeout_ms = timeout_ms
        self.start_time = time.time()

    def expired(self):
        return time.time() - self.start_time > self.timeout_ms / 1000.0


# ===================================================================================
# Error Class - Raise an Error
# ===================================================================================

class PrgError(Exception):
    def __init__(self, msg='Something went wrong'):
        super(PrgError, self).__init__(msg)
        sys.stderr.write('ERROR: ' + msg + '\n')
        sys.exit(1)


# ===================================================================================
# Device Definitions and Functions
# ===================================================================================

# Get list of supported device names
def get_supported_devices():
    result = list()
    for d in DEVICES:
        result.append(d['name'])
    return result

# Get device dictionary for a given device ID
def get_device(deviceid):
    for d in DEVICES:
        if d['device_id'] == deviceid:
            return d
    return None

# Device definitions
DEVICES = [
    {'name': 'attiny202',  'device_id': 0x1E9123, 'flash_size': 0x0800, 'flash_pagesize': 0x40},
    {'name': 'attiny402',  'device_id': 0x1E9227, 'flash_size': 0x1000, 'flash_pagesize': 0x40},
    {'name': 'attiny212',  'device_id': 0x1E9121, 'flash_size': 0x0800, 'flash_pagesize': 0x40},
    {'name': 'attiny412',  'device_id': 0x1E9223, 'flash_size': 0x1000, 'flash_pagesize': 0x40},
    {'name': 'attiny204',  'device_id': 0x1E9122, 'flash_size': 0x0800, 'flash_pagesize': 0x40},
    {'name': 'attiny404',  'device_id': 0x1E9226, 'flash_size': 0x1000, 'flash_pagesize': 0x40},
    {'name': 'attiny804',  'device_id': 0x1E9325, 'flash_size': 0x2000, 'flash_pagesize': 0x40},
    {'name': 'attiny1604', 'device_id': 0x1E9425, 'flash_size': 0x4000, 'flash_pagesize': 0x40},
    {'name': 'attiny214',  'device_id': 0x1E9120, 'flash_size': 0x0800, 'flash_pagesize': 0x40},
    {'name': 'attiny414',  'device_id': 0x1E9222, 'flash_size': 0x1000, 'flash_pagesize': 0x40},
    {'name': 'attiny814',  'device_id': 0x1E9322, 'flash_size': 0x2000, 'flash_pagesize': 0x40},
    {'name': 'attiny1614', 'device_id': 0x1E9422, 'flash_size': 0x4000, 'flash_pagesize': 0x40},
    {'name': 'attiny424',  'device_id': 0x1E922C, 'flash_size': 0x1000, 'flash_pagesize': 0x40},
    {'name': 'attiny824',  'device_id': 0x1E9329, 'flash_size': 0x2000, 'flash_pagesize': 0x40},
    {'name': 'attiny1624', 'device_id': 0x1E942A, 'flash_size': 0x4000, 'flash_pagesize': 0x40},
    {'name': 'attiny3224', 'device_id': 0x1E9528, 'flash_size': 0x8000, 'flash_pagesize': 0x80},
    {'name': 'attiny406',  'device_id': 0x1E9225, 'flash_size': 0x1000, 'flash_pagesize': 0x40},
    {'name': 'attiny806',  'device_id': 0x1E9324, 'flash_size': 0x2000, 'flash_pagesize': 0x40},
    {'name': 'attiny1606', 'device_id': 0x1E9424, 'flash_size': 0x4000, 'flash_pagesize': 0x40},
    {'name': 'attiny416',  'device_id': 0x1E9221, 'flash_size': 0x1000, 'flash_pagesize': 0x40},
    {'name': 'attiny816',  'device_id': 0x1E9321, 'flash_size': 0x2000, 'flash_pagesize': 0x40},
    {'name': 'attiny1616', 'device_id': 0x1E9421, 'flash_size': 0x4000, 'flash_pagesize': 0x40},
    {'name': 'attiny3216', 'device_id': 0x1E9521, 'flash_size': 0x8000, 'flash_pagesize': 0x80},
    {'name': 'attiny426',  'device_id': 0x1E922B, 'flash_size': 0x1000, 'flash_pagesize': 0x40},
    {'name': 'attiny826',  'device_id': 0x1E9328, 'flash_size': 0x2000, 'flash_pagesize': 0x40},
    {'name': 'attiny1626', 'device_id': 0x1E9429, 'flash_size': 0x4000, 'flash_pagesize': 0x40},
    {'name': 'attiny3226', 'device_id': 0x1E9527, 'flash_size': 0x8000, 'flash_pagesize': 0x80}
]


# ===================================================================================
# UPDI Protocol Constants
# ===================================================================================

UPDI_BREAK = 0x00
UPDI_LDS = 0x00
UPDI_STS = 0x40
UPDI_LD = 0x20
UPDI_ST = 0x60
UPDI_LDCS = 0x80
UPDI_STCS = 0xC0
UPDI_REPEAT = 0xA0
UPDI_KEY = 0xE0
UPDI_PTR = 0x00
UPDI_PTR_INC = 0x04
UPDI_PTR_ADDRESS = 0x08
UPDI_ADDRESS_8 = 0x00
UPDI_ADDRESS_16 = 0x04
UPDI_ADDRESS_24 = 0x08
UPDI_DATA_8 = 0x00
UPDI_DATA_16 = 0x01
UPDI_DATA_24 = 0x02

UPDI_PHY_SYNC = 0x55
UPDI_PHY_ACK = 0x40
UPDI_REPEAT_BYTE = 0x00
UPDI_REPEAT_WORD = 0x01
UPDI_CS_STATUSA = 0x00
UPDI_CS_STATUSB = 0x01
UPDI_CS_CTRLA = 0x02
UPDI_CS_CTRLB = 0x03
UPDI_CTRLA_RSD_BIT = 3
UPDI_CTRLA_IBDLY_BIT = 7
UPDI_CTRLB_CCDETDIS_BIT = 3
UPDI_CTRLB_UPDIDIS_BIT = 2
UPDI_ASI_KEY_STATUS = 0x07
UPDI_ASI_RESET_REQ = 0x08
UPDI_ASI_CTRLA = 0x09
UPDI_ASI_SYS_CTRLA = 0x0A
UPDI_ASI_SYS_STATUS = 0x0B
UPDI_ASI_CRC_STATUS = 0x0C
UPDI_ASI_STATUSA_REVID = 4
UPDI_ASI_STATUSB_PESIG = 0
UPDI_ASI_KEY_STATUS_CHIPERASE = 3
UPDI_ASI_KEY_STATUS_NVMPROG = 4
UPDI_ASI_KEY_STATUS_UROWWRITE = 5
UPDI_ASI_SYS_STATUS_RSTSYS = 5
UPDI_ASI_SYS_STATUS_INSLEEP = 4
UPDI_ASI_SYS_STATUS_NVMPROG = 3
UPDI_ASI_SYS_STATUS_UROWPROG = 2
UPDI_ASI_SYS_STATUS_LOCKSTATUS = 0
UPDI_ASI_SYS_CTRLA_UROW_FINAL = 1
UPDI_RESET_REQ_VALUE = 0x59
UPDI_MAX_REPEAT_SIZE = (0xFF+1)

UPDI_KEY_SIB = 0x04
UPDI_KEY_KEY = 0x00
UPDI_KEY_64 = 0x00
UPDI_KEY_128 = 0x01
UPDI_KEY_256 = 0x02
UPDI_KEY_NVM = b"NVMProg "
UPDI_KEY_CHIPERASE = b"NVMErase"
UPDI_KEY_UROW = b"NVMUs&te"
UPDI_SIB_8BYTES = UPDI_KEY_64
UPDI_SIB_16BYTES = UPDI_KEY_128
UPDI_SIB_32BYTES = UPDI_KEY_256

UPDI_NVMCTRL_CTRLA = 0x00
UPDI_NVMCTRL_CTRLB = 0x01
UPDI_NVMCTRL_STATUS = 0x02
UPDI_NVMCTRL_INTCTRL = 0x03
UPDI_NVMCTRL_INTFLAGS = 0x04
UPDI_NVMCTRL_DATAL = 0x06
UPDI_NVMCTRL_DATAH = 0x07
UPDI_NVMCTRL_ADDRL = 0x08
UPDI_NVMCTRL_ADDRH = 0x09

UPDI_V0_NVMCTRL_CTRLA_NOP = 0x00
UPDI_V0_NVMCTRL_CTRLA_WRITE_PAGE = 0x01
UPDI_V0_NVMCTRL_CTRLA_ERASE_PAGE = 0x02
UPDI_V0_NVMCTRL_CTRLA_ERASE_WRITE_PAGE = 0x03
UPDI_V0_NVMCTRL_CTRLA_PAGE_BUFFER_CLR = 0x04
UPDI_V0_NVMCTRL_CTRLA_CHIP_ERASE = 0x05
UPDI_V0_NVMCTRL_CTRLA_ERASE_EEPROM = 0x06
UPDI_V0_NVMCTRL_CTRLA_WRITE_FUSE = 0x07

UPDI_NVM_STATUS_WRITE_ERROR = 2
UPDI_NVM_STATUS_EEPROM_BUSY = 1
UPDI_NVM_STATUS_FLASH_BUSY = 0


# ===================================================================================

if __name__ == "__main__":
    _main()
