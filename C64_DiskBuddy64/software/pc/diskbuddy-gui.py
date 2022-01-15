#!/usr/bin/env python3
# ===================================================================================
# Project:   DiskBuddy64 - Python Script - Read Disk Image to D64 File
# Version:   v1.0
# Year:      2022
# Author:    Stefan Wagner
# Github:    https://github.com/wagiminator
# License:   http://creativecommons.org/licenses/by-sa/3.0/
# ===================================================================================
#
# Description:
# ------------
# Graphical front end for DiskBuddy64
#
# Dependencies:
# -------------
# - adapter  (included in libs folder)
# - tinyupdi (included in libs folder)
# - pyserial
# - tkinter (8.6 or newer)
#
# Operating Instructions:
# -----------------------
# - Set the serial mode switch on your DiskBuddy64 adapter to "UART".
# - Connect the adapter to your Commodore 1541(II) floppy disk drive via an IEC cable.
# - Connect the adapter to your PC via a USB cable.
# - Switch on your floppy disk drive.
# - Execute this skript: python diskbuddy-gui.py


import sys
import os
import time
from tkinter import *
from tkinter import messagebox, filedialog
from tkinter.ttk import *
from libs.adapter import *
from libs.tinyupdi import Programmer


# Variables
ramaddr   = 0x0500
filetypes = ['DEL', 'SEQ', 'PRG', 'USR', 'REL']

# Binary files
firmware   = 'libs/firmware.bin'
fastread   = 'libs/fastread.bin'
fastwrite  = 'libs/fastwrite.bin'
fastformat = 'libs/fastformat.bin'


# ===================================================================================
# Progress Box Class - Shows a Progress Bar
# ===================================================================================

class Progressbox(Toplevel):
    def __init__(self, root = None, title = 'Please wait !', 
                activity = 'Doing stuff ...', value = 0):
        Toplevel.__init__(self, root)
        self.__step = IntVar()
        self.__step.set(value)
        self.__act = StringVar()
        self.__act.set(activity)
        self.title(title)
        self.resizable(width=False, height=False)
        self.transient(root)
        self.grab_set()
        Label(self, textvariable = self.__act).pack(padx = 20, pady = 10)
        Progressbar(self, orient = HORIZONTAL, length = 200, 
                variable = self.__step, mode = 'determinate').pack(
                padx = 10, pady = 10)
        self.update_idletasks()

    def setactivity(self, activity):
        self.__act.set(activity)
        self.update_idletasks()

    def setvalue(self, value):
        if not value == self.__step.get():
            self.__step.set(value)
            self.update_idletasks()


# ===================================================================================
# Show File Content (HEX View)
# ===================================================================================

def showContent():
    fileName = filedialog.askopenfilename(title = 'Select file for HEX view',
                filetypes = (("D64 files","*.d64"), ("BIN files","*.bin"), ('All files','*.*')))
    if not fileName:
        return

    try:
        f = open(fileName, 'rb')
    except:
        messagebox.showerror('Error', 'Could not open file !')
        return

    fileSize = os.stat(fileName).st_size

    contentWindow = Toplevel(mainWindow)
    contentWindow.title('File content')
    contentWindow.minsize(200, 100)
    contentWindow.resizable(width=False, height=True)
    contentWindow.transient(mainWindow)
    contentWindow.grab_set()

    l = Listbox(contentWindow, font = 'TkFixedFont', height = 36, width = 73)
    l.pack(side='left', fill=BOTH)
    s = Scrollbar(contentWindow, orient = VERTICAL, command = l.yview)
    l['yscrollcommand'] = s.set
    s.pack(side='right', fill='y')

    startAddr = 0

    while (startAddr < fileSize):
        bytesline = '%06X: ' % startAddr
        asciiline = ' '
        i = 0
        while i < 16:
            if startAddr + i < fileSize:
                data = f.read(1)
                bytesline += '%02X ' % data[0]
                if (data[0] > 31): asciiline += chr(data[0])
                else: asciiline += '.'
            else:
                bytesline += '   '
                asciiline += ' '
            i += 1
        l.insert('end', bytesline + asciiline)
        startAddr += 16
    f.close()

    contentWindow.mainloop()
    contentWindow.quit()


# ===================================================================================
# Show Directory
# ===================================================================================


def diskDir():
    # Establish serial connection
    diskbuddy = Adapter()
    if not diskbuddy.is_open:
        messagebox.showerror('Error', 'DiskBuddy64 Adapter not found !')
        return

    # Check if IEC device ist present
    if not diskbuddy.checkdevice(device.get()):
        diskbuddy.close()
        messagebox.showerror('Error', 'IEC device ' + str(device.get()) + ' not found !')
        return

    # Upload fast loader to disk drive RAM
    if diskbuddy.uploadbin(ramaddr, fastread) > 0:
        diskbuddy.close()
        messagebox.showerror('Error', 'Uploading fastread.bin failed !')
        return

    # Read BAM
    dbam = BAM(diskbuddy.readblock(ramaddr, 18, 0))
    if not dbam.bam:
        diskbuddy.close()
        messagebox.showerror('Error', 'Reading BAM failed !')
        return

    # Draw content window
    contentWindow = Toplevel(mainWindow)
    contentWindow.title('Directory')
    contentWindow.minsize(200, 100)
    contentWindow.resizable(width=False, height=True)
    contentWindow.transient(mainWindow)
    contentWindow.grab_set()

    l = Listbox(contentWindow, font = 'TkFixedFont', height = 24, width = 32)
    l.pack(side='left', fill=BOTH)
    s = Scrollbar(contentWindow, orient = VERTICAL, command = l.yview)
    l['yscrollcommand'] = s.set
    s.pack(side='right', fill='y')

    # Print disk title
    l.insert('end', ' ' + dbam.getheader())

    # Get number of free blocks
    blocksfree = dbam.getblocksfree()

    # Print file entries
    track  = 18
    sector = 1
    while track > 0:
        block = diskbuddy.readblock(ramaddr, track, sector)
        if not block:
          diskbuddy.close()
          contentWindow.quit()
          messagebox.showerror('Error', 'Could not read from disk !')
          return

        track  = block[0]
        sector = block[1]
        ptr    = 0
        while ptr < 0xFF and block[ptr+0x02] > 0:
            line  = ' '
            line += str(int.from_bytes(block[ptr+0x1E:ptr+0x20], byteorder='little')).ljust(5)
            line += '\"'
            line += (PETtoASC(PETdelpadding(block[ptr+0x05:ptr+0x15])) + '\"').ljust(19)
            line += filetypes[block[ptr+0x02] & 0x07]
            l.insert('end', line.upper())
            ptr  += 0x20
    
    # Print free blocks
    l.insert('end', ' ' + str(blocksfree) + ' BLOCKS FREE.')

    diskbuddy.close()
    contentWindow.mainloop()
    contentWindow.quit()


# ===================================================================================
# Format Disk
# ===================================================================================


def diskFormat():
    # Draw parameter entry window
    parameterWindow = Toplevel(mainWindow)
    parameterWindow.title('Format Disk Parameters')
    parameterWindow.resizable(width=False, height=False)
    parameterWindow.transient(mainWindow)
    parameterWindow.grab_set()

    Label(parameterWindow, text='Disk title:').grid(row=0, sticky=W, padx=4, pady=4)
    ent1 = Entry(parameterWindow)
    ent1.insert(16, 'COMMODORE')
    ent1.grid(row=0, column=1, padx=4)

    Label(parameterWindow, text='Disk ident:').grid(row=1, sticky=W, padx=4, pady=4)
    ent2 = Entry(parameterWindow)
    ent2.insert(2, '64')
    ent2.grid(row=1, column=1, padx=4)

    var1 = IntVar()
    var2 = IntVar()
    var3 = IntVar()
    var2.set(1)
    Checkbutton(parameterWindow, text="Demagnetize the disk", variable=var1).grid(
                            row=2, columnspan=2, sticky=W, padx=10, pady=4)
    Checkbutton(parameterWindow, text="Bump the head", variable=var2).grid(
                            row=3, columnspan=2, sticky=W, padx=10, pady=4)
    Checkbutton(parameterWindow, text="Format 40 tracks", variable=var3).grid(
                            row=4, columnspan=2, sticky=W, padx=10, pady=4)

    Button(parameterWindow, text='OK', command=parameterWindow.quit).grid(
                            row=5, columnspan=2, sticky=EW, padx=4, pady=4)
    parameterWindow.mainloop()

    diskName  = ent1.get()
    diskIdent = ent2.get()
    demag     = var1.get()
    bump      = var2.get()
    if var3.get() == 1: tracks = 40
    else:               tracks = 35
    parameterWindow.destroy()

    if len(diskName) < 1 or len(diskName) > 16 or not len(diskIdent) == 2:
        messagebox.showerror('Error', 'Unsupported disk title or disk ident !')
        return

    # Establish serial connection
    diskbuddy = Adapter()
    if not diskbuddy.is_open:
        messagebox.showerror('Error', 'DiskBuddy64 Adapter not found !')
        return

    # Check if IEC device ist present
    if not diskbuddy.checkdevice(device.get()):
        diskbuddy.close()
        messagebox.showerror('Error', 'IEC device ' + str(device.get()) + ' not found !')
        return

    # Upload fast loader to disk drive RAM
    if diskbuddy.uploadbin(ramaddr, fastformat) > 0:
        diskbuddy.close()
        messagebox.showerror('Error', 'Uploading fastformat.bin failed !')
        return

    # Format the disk
    if diskbuddy.startformat(ramaddr, tracks, bump, demag, diskName, diskIdent) > 0:
        diskbuddy.close()
        messagebox.showerror('Error', 'Sending command failed !')
        return

    progress = Progressbox(mainWindow, 'DiskBuddy64 - Formatting disk', 'Formatting disk ...')
    starttime = time.time()
    diskbuddy.read(1)
    diskbuddy.timeout = 4
    for x in range(tracks):
        progr = diskbuddy.read(1)
        if not progr:
            diskbuddy.close()
            progress.destroy()
            messagebox.showerror('Error', 'Formatting disk failed !')
            return
        progress.setvalue((tracks - progr[0]) * 100 // tracks)

    # Finish all up
    duration = time.time() - starttime
    diskbuddy.close()
    progress.destroy()
    messagebox.showinfo('Mission accomplished', 
            'Formatting finished !\nDuration: ' + str(round(duration)) + ' sec')


# ===================================================================================
# Read Disk Image
# ===================================================================================


def diskRead():
    # Draw parameter entry window
    parameterWindow = Toplevel(mainWindow)
    parameterWindow.title('Copy Disk Parameters')
    parameterWindow.resizable(width=False, height=False)
    parameterWindow.transient(mainWindow)
    parameterWindow.grab_set()

    var1 = IntVar()
    var2 = IntVar()
    var1.set(1)
    Checkbutton(parameterWindow, text="Copy only blocks with BAM entry", variable=var1).grid(
                            row=0, sticky=W, padx=10, pady=4)
    Checkbutton(parameterWindow, text="Verify after copying", variable=var2).grid(
                            row=1, sticky=W, padx=10, pady=4)
    Button(parameterWindow, text='Select output file', command=parameterWindow.quit).grid(
                            row=2, columnspan=2, sticky=EW, padx=4, pady=4)
    parameterWindow.mainloop()

    bamcopy   = var1.get()
    verify    = var2.get()
    tracks    = 35
    allocated = 683
    parameterWindow.destroy()

    # Get output file
    filename = filedialog.asksaveasfilename(title = 'Select output file',
                filetypes = (("D64 files","*.d64"), ('All files','*.*')))
    if not filename:
        return

    # Establish serial connection
    diskbuddy = Adapter()
    if not diskbuddy.is_open:
        messagebox.showerror('Error', 'DiskBuddy64 Adapter not found !')
        return

    # Check if IEC device ist present
    if not diskbuddy.checkdevice(device.get()):
        diskbuddy.close()
        messagebox.showerror('Error', 'IEC device ' + str(device.get()) + ' not found !')
        return

    # Upload fast loader to disk drive RAM
    if diskbuddy.uploadbin(ramaddr, fastread) > 0:
        diskbuddy.close()
        messagebox.showerror('Error', 'Uploading fastread.bin failed !')
        return

    # Create output file
    try:
        f = open(filename, 'wb')
    except:
        diskbuddy.close()
        messagebox.showerror('Error', 'Could not create output file !')
        return

    # Fill output file with default values
    for track in range(1, tracks + 1):
        for sector in range(getsectors(track)):
            f.write(b'\x4B' + b'\x01' * 255)

    # Read BAM if necessary
    progress = Progressbox(mainWindow, 'DiskBuddy64 - Reading disk', 'Copy disk to D64 file ...')
    if bamcopy > 0:
        dbam = BAM(diskbuddy.readblock(ramaddr, 18, 0))
        if not dbam.bam:
            f.close()
            diskbuddy.close()
            progress.destroy()
            messagebox.showerror('Error', 'Could not read BAM !')
            return
        allocated = dbam.getallocated()

    # Read disk
    copied = 0
    starttime = time.time()
    for track in range(1, tracks + 1):
        secnum      = getsectors(track)
        interleave  = secnum // 2;
        sectors     = [x for x in range(secnum)]
        seclist     = []

        # Cancel sectors without BAM entry
        if bamcopy > 0 and track < 36:
            for x in range(secnum):
                if dbam.blockisfree(track, x): sectors.remove(x)

        # Optimize order of sectors for speed
        sector  = 0
        counter = len(sectors)
        while counter:
            if sector >= secnum: sector -= secnum
            while not sector in sectors:
                sector += 1
                if sector >= secnum: sector = 0
            seclist.append(sector)
            sectors.remove(sector)
            sector  += interleave
            counter -= 1

        # Send command to disk drive, if there's something to read on track
        progress.setactivity('Copy disk to D64 file ...' +
                '\nTrack: ' + str(track) + ' of ' + str(tracks))
        seclen = len(seclist)
        if seclen > 0:
            if diskbuddy.startfastiec('r', ramaddr, track, seclist) > 0:
                f.close()
                diskbuddy.close()
                progress.destroy()
                messagebox.showerror('Error', 'Could not read from disk !')
                return

        # Read track
        diskbuddy.timeout = 3
        for sector in seclist:
            f.seek(getfilepointer(track, sector))
            if not f.write(diskbuddy.read(256)) == 256:
                f.close()
                diskbuddy.close()
                progress.destroy()
                messagebox.showerror('Error', 'Could not read from disk !')
                return
            diskbuddy.timeout = 1
            copied += 1
        progress.setvalue(copied * 100 // allocated)

    # Finish all up
    duration = time.time() - starttime
    f.close()
    diskbuddy.close()
    progress.destroy()
    if verify > 0:
        diskVerify(filename, bamcopy, tracks)
    else:
        messagebox.showinfo('Mission accomplished', 
            'Copying finished !\nDuration: ' + str(round(duration)) + ' sec')


# ===================================================================================
# Write Disk Image
# ===================================================================================


def diskWrite():
    # Draw parameter entry window
    parameterWindow = Toplevel(mainWindow)
    parameterWindow.title('Copy Disk Parameters')
    parameterWindow.resizable(width=False, height=False)
    parameterWindow.transient(mainWindow)
    parameterWindow.grab_set()

    var1 = IntVar()
    var2 = IntVar()
    var1.set(1)
    Checkbutton(parameterWindow, text="Copy only blocks with BAM entry", variable=var1).grid(
                            row=0, sticky=W, padx=10, pady=4)
    Checkbutton(parameterWindow, text="Verify after copying", variable=var2).grid(
                            row=1, sticky=W, padx=10, pady=4)
    Button(parameterWindow, text='Select input file', command=parameterWindow.quit).grid(
                            row=2, columnspan=2, sticky=EW, padx=4, pady=4)
    parameterWindow.mainloop()

    bamcopy   = var1.get()
    verify    = var2.get()
    tracks    = 35
    allocated = 683
    parameterWindow.destroy()

    # Get output file
    filename = filedialog.askopenfilename(title = 'Select input file',
                filetypes = (("D64 files","*.d64"), ('All files','*.*')))
    if not filename:
        return

    # Establish serial connection
    diskbuddy = Adapter()
    if not diskbuddy.is_open:
        messagebox.showerror('Error', 'DiskBuddy64 Adapter not found !')
        return

    # Check if IEC device ist present
    if not diskbuddy.checkdevice(device.get()):
        diskbuddy.close()
        messagebox.showerror('Error', 'IEC device ' + str(device.get()) + ' not found !')
        return

    # Upload fast loader to disk drive RAM
    if diskbuddy.uploadbin(ramaddr, fastwrite) > 0:
        diskbuddy.close()
        messagebox.showerror('Error', 'Uploading fastwrite.bin failed !')
        return

    # Open input file
    try:
        filesize = os.stat(filename).st_size
        f = open(filename, 'rb')
    except:
        diskbuddy.close()
        messagebox.showerror('Error', 'Could not open input file !')
        return

    # Check input file
    if not filesize == getfilepointer(tracks + 1, 0):
        f.close()
        diskbuddy.close()
        messagebox.showerror('Error', 'Wrong input file size !')
        return

    # Read BAM if necessary
    if bamcopy > 0:
        f.seek(getfilepointer(18, 0))
        fbam = BAM(f.read(256))
        allocated = fbam.getallocated()

    # Write disk
    copied = 0
    progress = Progressbox(mainWindow, 'DiskBuddy64 - Writing disk', 'Copy D64 file to disk ...')
    starttime = time.time()
    for track in range(1, tracks + 1):
        secnum      = getsectors(track)
        sectors     = [x for x in range(secnum)]

        # Cancel sectors without BAM entry
        if bamcopy > 0 and track < 36:
            for x in range(secnum):
                if fbam.blockisfree(track, x): sectors.remove(x)

        # Send command to disk drive, if there's something to write on track
        progress.setactivity('Copy D64 file to disk ...' +
                '\nTrack: ' + str(track) + ' of ' + str(tracks))
        seclen = len(sectors)
        if seclen > 0:
            if diskbuddy.startfastiec('w', ramaddr, track, sectors) > 0:
                f.close()
                diskbuddy.close()
                progress.destroy()
                messagebox.showerror('Error', 'Could not write to disk !')
                return

        # Write track
        diskbuddy.timeout = 3
        for sector in sectors:
            f.seek(getfilepointer(track, sector))
            counter = 256;
            while counter > 0:
                requested = diskbuddy.read(1);
                if not requested:
                    f.close()
                    diskbuddy.close()
                    progress.destroy()
                    messagebox.showerror('Error', 'Could not write to disk !')
                    return
                diskbuddy.write(f.read(requested[0]))
                counter -= requested[0]
            copied += 1
        if seclen > 0:  diskbuddy.read(1);
        progress.setvalue(copied * 100 // allocated)

    # Finish all up
    duration = time.time() - starttime
    f.close()
    diskbuddy.close()
    progress.destroy()
    if verify > 0:
        diskVerify(filename, bamcopy, tracks)
    else:
        messagebox.showinfo('Mission accomplished', 
            'Copying finished !\nDuration: ' + str(round(duration)) + ' sec')


# ===================================================================================
# Verify Disk Image
# ===================================================================================


def diskVerify(filename, bamcopy, tracks):
    # Establish serial connection
    diskbuddy = Adapter()
    if not diskbuddy.is_open:
        messagebox.showerror('Error', 'DiskBuddy64 Adapter not found !')
        return

    # Upload fast loader to disk drive RAM
    if diskbuddy.uploadbin(ramaddr, fastread) > 0:
        diskbuddy.close()
        messagebox.showerror('Error', 'Uploading fastread.bin failed !')
        return

    # Open image file
    try:
        f = open(filename, 'rb')
    except:
        diskbuddy.close()
        messagebox.showerror('Error', 'Could not open image file !')
        return

    # Read BAM if necessary
    progress = Progressbox(mainWindow, 'DiskBuddy64 - Verifying', 'Verify disk image ...')
    allocated = 683
    if bamcopy > 0:
        dbam = BAM(diskbuddy.readblock(ramaddr, 18, 0))
        if not dbam.bam:
            f.close()
            diskbuddy.close()
            progress.destroy()
            messagebox.showerror('Error', 'Could not open BAM !')
            return
        f.seek(getfilepointer(18, 0))
        fbam = BAM(f.read(256))
        if not dbam.bam == fbam.bam:
            f.close()
            diskbuddy.close()
            progress.destroy()
            messagebox.showerror('Error', 'BAM mismatch !')
            return
        allocated = dbam.getallocated()

    # Read disk
    errors    = 0
    verified  = 0
    for track in range(1, tracks + 1):
        secnum      = getsectors(track)
        interleave  = secnum // 2;
        sectors     = [x for x in range(secnum)]
        seclist     = []

        # Cancel sectors without BAM entry
        if bamcopy > 0 and track < 36:
            for x in range(secnum):
                if dbam.blockisfree(track, x): sectors.remove(x)

        # Optimize order of sectors for speed
        sector  = 0
        counter = len(sectors)
        while counter:
            if sector >= secnum: sector -= secnum
            while not sector in sectors:
                sector += 1
                if sector >= secnum: sector = 0
            seclist.append(sector)
            sectors.remove(sector)
            sector  += interleave
            counter -= 1

        # Send command to disk drive, if there's something to read on track
        seclen = len(seclist)
        if seclen > 0:
            if diskbuddy.startfastiec('r', ramaddr, track, seclist) > 0:
                f.close()
                diskbuddy.close()
                progress.destroy()
                messagebox.showerror('Error', 'Could not read from disk !')
                return

        # Read track
        diskbuddy.timeout = 3
        for sector in seclist:
            f.seek(getfilepointer(track, sector))
            fblock = f.read(256)
            block  = diskbuddy.read(256)
            if not block:
                f.close()
                diskbuddy.close()
                progress.destroy()
                messagebox.showerror('Error', 'Could not read from disk !')
                return
            if not fblock == block:
                errors += 1
            verified += 1
            diskbuddy.timeout = 1
        progress.setvalue(verified * 100 // allocated)
        progress.setactivity('Verify disk image ...' +
                '\nTrack: ' + str(track) + ' of ' + str(tracks) +
                '\nSectors: ' + str(verified) + ' of ' + str(allocated) +
                '\nErrors: ' + str(errors))

    # Finish all up
    f.close()
    diskbuddy.close()
    progress.destroy()
    messagebox.showinfo('Mission accomplished', 
            'Verifying finished !\nErrors: ' + str(errors))


# ===================================================================================
# Flash Firmware
# ===================================================================================


def flashFirmware():
    # Valid target MCUs
    targets = {
        0x1E9123: 'ATtiny202',
        0x1E9227: 'ATtiny402',
        0x1E9121: 'ATtiny212',
        0x1E9223: 'ATtiny412'
    }

    # Fuse settings
    fuses = {0: 0x00, 1: 0x00, 2: 0x01, 4: 0x00, 5: 0xC5, 6: 0x04, 7: 0x00, 8: 0x00}

    # Show info
    messagebox.showinfo('Flash Firmware', 
            'Set the serial mode switch to "UPDI" and klick "OK"!')

    # Establish serial connection
    progress = Progressbox(mainWindow, 'DiskBuddy64 - Flashing firmware', 'Connecting to device ...')
    tinyupdi = Programmer()
    if not tinyupdi.is_open:
        progress.destroy()
        messagebox.showerror('Error', 'Device not found!')
        return

    # Enter progmode
    progress.setactivity('Pinging target MCU ...')
    progress.setvalue(30)
    try:
        tinyupdi.enter_progmode()
    except:
        tinyupdi.unlock()

    # Read device ID, identify target MCU
    try:
        devid = tinyupdi.get_device_id()
        if not devid in targets:
            raise Exception()
    except:
        tinyupdi.close()
        progress.destroy()
        messagebox.showerror('Error', 'Unknown or unsupported device!')
        return

    # Flash firmware
    progress.setactivity('Flashing and verifying firmware ...')
    progress.setvalue(60)
    try:
        if not tinyupdi.flash_bin(firmware):
            raise Exception()
    except:
        tinyupdi.close()
        progress.destroy()
        messagebox.showerror('Error', 'Flashing firmware failed!')
        return

    # Burning fuses
    progress.setactivity('Writing and verifying fuses ...')
    progress.setvalue(90)
    try:
        for f in fuses:
            if not tinyupdi.set_fuse(f, fuses[f]):
                Exception()
    except:
        tinyupdi.close()
        progress.destroy()
        messagebox.showerror('Error', 'Writing fuses failed!')
        return

    progress.destroy()
    tinyupdi.leave_progmode()
    tinyupdi.close()

    # Show info
    messagebox.showinfo('Mission accomplished', 
            'Firmware successfully flashed.\nSet the serial mode switch back to "UART"!')


# ===================================================================================
# Main Function
# ===================================================================================

mainWindow = Tk()
mainWindow.title('DiskBuddy64')
mainWindow.resizable(width=False, height=False)

device = IntVar()
device.set(8)
devices = [('#8',8), ('#9',9), ('#10',10), ('#11',11)]

deviceFrame = Frame(mainWindow, borderwidth = 2, relief = 'groove')
Label(deviceFrame, text = '1. Select device number:').grid(row=0,columnspan=4,pady=5)
for txt, val in devices:
    Radiobutton(deviceFrame, text=txt, variable=device, value=val).grid(row=1,column=val-8,padx=5)
deviceFrame.pack(padx = 10, pady = 10, ipadx = 5, ipady = 5, fill = 'x')

actionFrame = Frame(mainWindow, borderwidth = 2, relief = 'groove')
Label(actionFrame, text = '2. Take your action:').pack(pady = 5)
Button(actionFrame, text = 'Format disk', command = diskFormat
            ).pack(padx = 10, pady = 2, fill = 'x')
Button(actionFrame, text = 'Copy disk to D64 file', command = diskRead,
            ).pack(padx = 10, pady = 2, fill = 'x')
Button(actionFrame, text = 'Copy D64 file to disk', command = diskWrite,
            ).pack(padx = 10, pady = 2, fill = 'x')
Button(actionFrame, text = 'Show disk directory', command = diskDir,
            ).pack(padx = 10, pady = 2, fill = 'x')
actionFrame.pack(padx =10, ipadx = 5, ipady = 5, fill = 'x')

specialFrame = Frame(mainWindow, borderwidth = 2, relief = 'groove')
Label(specialFrame, text = '3. Additional Functions:').pack(pady = 5)
Button(specialFrame, text = 'Show file content (hex)', command = showContent,
            ).pack(padx = 10, pady = 2, fill = 'x')
Button(specialFrame, text = 'Flash firmware', command = flashFirmware,
            ).pack(padx = 10, pady = 2, fill = 'x')
specialFrame.pack(padx =10, pady = 10, ipadx = 5, ipady = 5, fill = 'x')

Button(mainWindow, text = 'Exit', command = mainWindow.quit).pack(pady = 10)

mainWindow.mainloop()
