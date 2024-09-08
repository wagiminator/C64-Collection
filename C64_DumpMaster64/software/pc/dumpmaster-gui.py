#!/usr/bin/env python3
# ===================================================================================
# Project:   DumpMaster64 - Python Script - Read Disk Image to D64 File
# Version:   v1.3.2
# Year:      2022
# Author:    Stefan Wagner
# Github:    https://github.com/wagiminator
# License:   http://creativecommons.org/licenses/by-sa/3.0/
# ===================================================================================
#
# Description:
# ------------
# Graphical front end for DumpMaster64
#
# Dependencies:
# -------------
# - adapter   (included in libs folder)
# - disktools (included in libs folder)
# - tinyupdi  (included in libs folder)
# - pyserial
# - tkinter (8.6 or newer)
#
# Operating Instructions:
# -----------------------
# - Set the serial mode switch on your DumpMaster64 adapter to "UART".
# - Connect the adapter to your Datasette and/or your floppy disk drive(s).
# - Connect the adapter to a USB port of your PC.
# - Switch on your floppy disk drive(s) if connected.
# - Execute the desired Python script on your PC to interface the adapter.


import sys
import os
import time
from tkinter import *
from tkinter import messagebox, filedialog
from tkinter.ttk import *
from libs.adapter import *
from libs.disktools import *
from libs.tinyupdi import Programmer


# Binary Files
FIRMWARE_BIN   = 'libs/firmware.bin'
FASTREAD_BIN   = 'libs/fastread.bin'
FASTLOAD_BIN   = 'libs/fastload.bin'
FASTWRITE_BIN  = 'libs/fastwrite.bin'
FASTUPLOAD_BIN = 'libs/fastupload.bin'
FASTFORMAT_BIN = 'libs/fastformat.bin'

# Default variables
interleave = 4
trackgap   = 6


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
        self.update()

    def setactivity(self, activity):
        self.__act.set(activity)
        self.update()

    def setvalue(self, value):
        if not value == self.__step.get():
            self.__step.set(value)
            self.update()


# ===================================================================================
# Show Disk Directory
# ===================================================================================

def diskDir():
    # Establish serial connection
    dumpmaster = Adapter()
    if not dumpmaster.is_open:
        messagebox.showerror('Error', 'DumpMaster64 Adapter not found !')
        return

    # Check if IEC device ist present
    if not dumpmaster.checkdevice(device.get()):
        dumpmaster.close()
        messagebox.showerror('Error', 'IEC device ' + str(device.get()) + ' not found !')
        return

    # Upload fast loader to disk drive RAM
    if dumpmaster.uploadbin(FASTUPLOAD_LOADADDR, FASTUPLOAD_BIN) > 0 \
    or dumpmaster.fastuploadbin(FASTREAD_LOADADDR, FASTREAD_BIN) > 0:
        dumpmaster.close()
        messagebox.showerror('Error', 'Failed to upload fastread.bin !')
        return

    # Read BAM
    dbam = BAM(dumpmaster.readblock(18, 0))
    if not dbam.bam:
        dumpmaster.close()
        messagebox.showerror('Error', 'Failed to read the BAM !')
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
        block = dumpmaster.readblock(track, sector)
        if not block:
          dumpmaster.close()
          contentWindow.quit()
          messagebox.showerror('Error', 'Failed to read from disk !')
          return

        track  = block[0]
        sector = block[1]
        ptr    = 0
        while ptr < 0xFF and block[ptr+0x02] > 0:
            line  = ' '
            line += str(int.from_bytes(block[ptr+0x1E:ptr+0x20], byteorder='little')).ljust(5)
            line += '\"'
            line += (PETtoASC(PETdelpadding(block[ptr+0x05:ptr+0x15])) + '\"').ljust(19)
            line += FILETYPES[block[ptr+0x02] & 0x07]
            if (block[ptr+0x02] & 0x40) > 0:  line += '<'
            if (block[ptr+0x02] & 0x80) == 0: line += '*'
            l.insert('end', line.upper())
            ptr  += 0x20
    
    # Print free blocks
    l.insert('end', ' ' + str(blocksfree) + ' BLOCKS FREE.')

    dumpmaster.close()
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

    Checkbutton(parameterWindow, text="Demagnetize the disk", variable=f_demag_var).grid(
                            row=2, columnspan=2, sticky=W, padx=10, pady=4)
    Checkbutton(parameterWindow, text="Bump the head", variable=f_bump_var).grid(
                            row=3, columnspan=2, sticky=W, padx=10, pady=4)
    Checkbutton(parameterWindow, text="Format 40 tracks", variable=f_extend_var).grid(
                            row=4, columnspan=2, sticky=W, padx=10, pady=4)
    Checkbutton(parameterWindow, text="Verify on the fly", variable=f_verify_var).grid(
                            row=5, columnspan=2, sticky=W, padx=10, pady=4)

    Button(parameterWindow, text='Start formatting', command=parameterWindow.quit).grid(
                            row=6, columnspan=2, sticky=EW, padx=4, pady=4)
    parameterWindow.mainloop()

    diskName  = ent1.get().upper()
    diskIdent = ent2.get().upper()
    demag     = f_demag_var.get()
    bump      = f_bump_var.get()
    verify    = f_verify_var.get()
    if f_extend_var.get() == 1: tracks = 40
    else:                       tracks = 35
    parameterWindow.destroy()

    if len(diskName) < 1 or len(diskName) > 16 or not len(diskIdent) == 2:
        messagebox.showerror('Error', 'Unsupported disk title or disk ident !')
        return

    # Establish serial connection
    dumpmaster = Adapter()
    if not dumpmaster.is_open:
        messagebox.showerror('Error', 'DumpMaster64 Adapter not found !')
        return

    # Check if IEC device ist present
    if not dumpmaster.checkdevice(device.get()):
        dumpmaster.close()
        messagebox.showerror('Error', 'IEC device ' + str(device.get()) + ' not found !')
        return

    # Upload fast loader to disk drive RAM
    if dumpmaster.uploadbin(FASTUPLOAD_LOADADDR, FASTUPLOAD_BIN) > 0 \
    or dumpmaster.fastuploadbin(FASTFORMAT_LOADADDR, FASTFORMAT_BIN) > 0:
        dumpmaster.close()
        messagebox.showerror('Error', 'Failed to upload fastformat.bin !')
        return

    # Format the disk
    if dumpmaster.startfastformat(tracks, bump, demag, verify, diskName, diskIdent) > 0:
        dumpmaster.close()
        messagebox.showerror('Error', 'Failed to send command !')
        return

    progress = Progressbox(mainWindow, 'Formatting disk', 'Formatting disk ...')
    starttime = time.time()
    dumpmaster.timeout = 4
    for x in range(tracks + 1):
        progr = dumpmaster.read(1)
        if not progr or progr[0] > 0:
            dumpmaster.close()
            progress.destroy()
            messagebox.showerror('Error', 'Failed to format the disk !')
            return
        progress.setvalue(x * 100 // tracks)

    # Finish all up
    duration = time.time() - starttime
    dumpmaster.close()
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

    r_extend_var = IntVar()
    Checkbutton(parameterWindow, text="Copy only blocks with BAM entry", variable=r_bam_var).grid(
                            row=0, sticky=W, padx=10, pady=4)
    Checkbutton(parameterWindow, text="Copy 40 tracks", variable=r_extend_var).grid(
                            row=1, sticky=W, padx=10, pady=4)
    Checkbutton(parameterWindow, text="Verify after copying", variable=r_verify_var).grid(
                            row=2, sticky=W, padx=10, pady=4)
    Button(parameterWindow, text='Select output file', command=parameterWindow.quit).grid(
                            row=3, columnspan=2, sticky=EW, padx=4, pady=4)
    parameterWindow.mainloop()

    bamcopy = r_bam_var.get()
    verify  = r_verify_var.get()
    if r_extend_var.get() > 0:
        tracks    = 40
        allocated = 768
    else:
        tracks    = 35
        allocated = 683
    parameterWindow.destroy()

    # Get output file
    filename = filedialog.asksaveasfilename(title = 'Select output file',
                filetypes = (("D64 files","*.d64"), ('All files','*.*')))
    if not filename:
        return

    # Establish serial connection
    dumpmaster = Adapter()
    if not dumpmaster.is_open:
        messagebox.showerror('Error', 'DumpMaster64 Adapter not found !')
        return

    # Check if IEC device ist present
    if not dumpmaster.checkdevice(device.get()):
        dumpmaster.close()
        messagebox.showerror('Error', 'IEC device ' + str(device.get()) + ' not found !')
        return

    # Upload fast loader to disk drive RAM
    if dumpmaster.uploadbin(FASTUPLOAD_LOADADDR, FASTUPLOAD_BIN) > 0 \
    or dumpmaster.fastuploadbin(FASTREAD_LOADADDR, FASTREAD_BIN) > 0:
        dumpmaster.close()
        messagebox.showerror('Error', 'Failed to upload fastread.bin !')
        return

    # Create output file
    try:
        f = open(filename, 'wb')
    except:
        dumpmaster.close()
        messagebox.showerror('Error', 'Failed to create output file !')
        return

    # Fill output file with default values
    for track in range(1, tracks + 1):
        for sector in range(getsectors(track)):
            f.write(b'\x4B' + b'\x01' * 255)

    # Read BAM if necessary
    progress = Progressbox(mainWindow, 'Reading from disk', 'Copy disk to D64 file ...')
    if bamcopy > 0:
        dbam = BAM(dumpmaster.readblock(18, 0))
        if not dbam.bam:
            f.close()
            dumpmaster.close()
            progress.destroy()
            messagebox.showerror('Error', 'Failed to read the BAM !')
            return
        allocated = dbam.getallocated()
        if tracks > 35:
            allocated += 85

    # Read disk
    copied = 0
    errors = 0
    starttime = time.time()
    for track in range(1, tracks + 1):
        secnum  = getsectors(track)
        sectors = [x for x in range(secnum)]
        seclist = []

        # Cancel sectors without BAM entry
        if bamcopy > 0 and track < 36:
            for x in range(secnum):
                if dbam.blockisfree(track, x): sectors.remove(x)

        # Optimize order of sectors for speed
        sector  = trackgap * (track - 1)
        counter = len(sectors)
        while counter:
            sector %= secnum
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
            if dumpmaster.startfastread(track, seclist) > 0:
                f.close()
                dumpmaster.close()
                progress.destroy()
                messagebox.showerror('Error', 'Failed to read from disk !')
                return

        # Read track
        dumpmaster.timeout = 3
        for sector in seclist:
            f.seek(getfilepointer(track, sector))
            block = dumpmaster.getblockgcr()
            if not block:
                f.close()
                dumpmaster.close()
                progress.destroy()
                messagebox.showerror('Error', 'Failed to read from disk !')
                return
            if not len(block) == 256:
                errors += 1
            else:
                f.write(block)
            dumpmaster.timeout = 1
            copied += 1
        progress.setvalue(copied * 100 // allocated)

    # Finish all up
    if verify == 0:
        dumpmaster.executememory(MEMCMD_SETTRACK18)
    duration = time.time() - starttime
    f.close()
    dumpmaster.close()
    progress.destroy()
    if verify > 0:
        diskVerify(filename, bamcopy, tracks)
    else:
        messagebox.showinfo('Mission accomplished', 
            'Copying finished !\nRead Errors: ' + str(errors) + 
            '\nDuration: ' + str(round(duration)) + ' sec')


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

    Checkbutton(parameterWindow, text="Copy only blocks with BAM entry", variable=w_bam_var).grid(
                            row=0, sticky=W, padx=10, pady=4)
    Checkbutton(parameterWindow, text="Verify after copying", variable=w_verify_var).grid(
                            row=1, sticky=W, padx=10, pady=4)
    Button(parameterWindow, text='Select input file', command=parameterWindow.quit).grid(
                            row=2, columnspan=2, sticky=EW, padx=4, pady=4)
    parameterWindow.mainloop()

    bamcopy   = w_bam_var.get()
    verify    = w_verify_var.get()
    tracks    = 35
    allocated = 683
    parameterWindow.destroy()

    # Get output file
    filename = filedialog.askopenfilename(title = 'Select input file',
                filetypes = (("D64 files","*.d64"), ('All files','*.*')))
    if not filename:
        return

    # Establish serial connection
    dumpmaster = Adapter()
    if not dumpmaster.is_open:
        messagebox.showerror('Error', 'DumpMaster64 Adapter not found !')
        return

    # Check if IEC device ist present
    if not dumpmaster.checkdevice(device.get()):
        dumpmaster.close()
        messagebox.showerror('Error', 'IEC device ' + str(device.get()) + ' not found !')
        return

    # Upload fast loader to disk drive RAM
    if dumpmaster.uploadbin(FASTUPLOAD_LOADADDR, FASTUPLOAD_BIN) > 0 \
    or dumpmaster.fastuploadbin(FASTWRITE_LOADADDR, FASTWRITE_BIN) > 0:
        dumpmaster.close()
        messagebox.showerror('Error', 'Failed to upload fastwrite.bin !')
        return

    # Open input file
    try:
        filesize = os.stat(filename).st_size
        f = open(filename, 'rb')
    except:
        dumpmaster.close()
        messagebox.showerror('Error', 'Failed to open input file !')
        return

    # Check input file
    if not filesize == getfilepointer(tracks + 1, 0):
        if filesize == getfilepointer(41, 0):
            messagebox.showinfo('Warning', 
                'This is a disk image with 40 tracks!')
            tracks    = 40
            allocated = 768
        else:
            f.close()
            dumpmaster.close()
            messagebox.showerror('Error', 'Wrong input file size !')
            return

    # Read BAM if necessary
    if bamcopy > 0:
        f.seek(getfilepointer(18, 0))
        fbam = BAM(f.read(256))
        allocated = fbam.getallocated()
        if tracks > 35: allocated += 85

    # Write disk
    copied = 0
    progress = Progressbox(mainWindow, 'Writing to disk', 'Copy D64 file to disk ...')
    starttime = time.time()
    for track in range(1, tracks + 1):
        secnum  = getsectors(track)
        sectors = [x for x in range(secnum)]
        seclist = []

        # Cancel sectors without BAM entry
        if bamcopy > 0 and track < 36:
            for x in range(secnum):
                if fbam.blockisfree(track, x): sectors.remove(x)

        # Optimize order of sectors for speed
        sector  = trackgap * (track - 1)
        counter = len(sectors)
        while counter:
            sector %= secnum
            while not sector in sectors:
                sector += 1
                if sector >= secnum: sector = 0
            seclist.append(sector)
            sectors.remove(sector)
            sector  += interleave
            counter -= 1

        # Send command to disk drive, if there's something to write on track
        progress.setactivity('Copy D64 file to disk ...' +
                '\nTrack: ' + str(track) + ' of ' + str(tracks))
        seclen = len(seclist)
        if seclen > 0:
            if dumpmaster.startfastwrite(track, seclist) > 0:
                f.close()
                dumpmaster.close()
                progress.destroy()
                messagebox.showerror('Error', 'Failed to write on disk !')
                return

        # Write track
        dumpmaster.timeout = 3
        for sector in seclist:
            f.seek(getfilepointer(track, sector))
            if dumpmaster.sendblockgcr(f.read(256)) > 0:
                f.close()
                dumpmaster.close()
                progress.destroy()
                messagebox.showerror('Error', 'Failed to write on disk !')
                return
            copied += 1
        if seclen > 0:  dumpmaster.read(1);
        progress.setvalue(copied * 100 // allocated)

    # Finish all up
    if verify == 0:
        dumpmaster.executememory(MEMCMD_SETTRACK18)
    duration = time.time() - starttime
    f.close()
    dumpmaster.close()
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
    dumpmaster = Adapter()
    if not dumpmaster.is_open:
        messagebox.showerror('Error', 'DumpMaster64 Adapter not found !')
        return

    # Upload fast loader to disk drive RAM
    if dumpmaster.uploadbin(FASTUPLOAD_LOADADDR, FASTUPLOAD_BIN) > 0 \
    or dumpmaster.fastuploadbin(FASTREAD_LOADADDR, FASTREAD_BIN) > 0:
        dumpmaster.close()
        messagebox.showerror('Error', 'Failed to upload fastread.bin !')
        return

    # Open image file
    try:
        f = open(filename, 'rb')
    except:
        dumpmaster.close()
        messagebox.showerror('Error', 'Failed to open image file !')
        return

    # Read BAM if necessary
    progress = Progressbox(mainWindow, 'Verifying disk', 'Verify disk image ...')
    allocated = getsectornumber(tracks + 1, 0)
    if bamcopy > 0:
        dbam = BAM(dumpmaster.readblock(18, 0))
        if not dbam.bam:
            f.close()
            dumpmaster.close()
            progress.destroy()
            messagebox.showerror('Error', 'Failed to open BAM !')
            return
        f.seek(getfilepointer(18, 0))
        fbam = BAM(f.read(256))
        if not dbam.bam == fbam.bam:
            f.close()
            dumpmaster.close()
            progress.destroy()
            messagebox.showerror('Error', 'BAM mismatch !')
            return
        allocated = dbam.getallocated()
        if tracks > 35:
            allocated += 85

    # Read disk
    errors    = 0
    verified  = 0
    for track in range(1, tracks + 1):
        secnum  = getsectors(track)
        sectors = [x for x in range(secnum)]
        seclist = []

        # Cancel sectors without BAM entry
        if bamcopy > 0 and track < 36:
            for x in range(secnum):
                if dbam.blockisfree(track, x): sectors.remove(x)

        # Optimize order of sectors for speed
        sector  = trackgap * (track - 1)
        counter = len(sectors)
        while counter:
            sector %= secnum
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
            if dumpmaster.startfastread(track, seclist) > 0:
                f.close()
                dumpmaster.close()
                progress.destroy()
                messagebox.showerror('Error', 'Failed to read from disk !')
                return

        # Read track
        dumpmaster.timeout = 3
        for sector in seclist:
            f.seek(getfilepointer(track, sector))
            fblock = f.read(256)
            block  = dumpmaster.getblockgcr()
            if not block:
                f.close()
                dumpmaster.close()
                progress.destroy()
                messagebox.showerror('Error', 'Failed to read from disk !')
                return
            if not fblock == block:
                errors += 1
            verified += 1
            dumpmaster.timeout = 1
        progress.setvalue(verified * 100 // allocated)
        progress.setactivity('Verify disk image ...' +
                '\nTrack: ' + str(track) + ' of ' + str(tracks) +
                '\nSectors: ' + str(verified) + ' of ' + str(allocated) +
                '\nErrors: ' + str(errors))

    # Finish all up
    dumpmaster.executememory(MEMCMD_SETTRACK18)
    f.close()
    dumpmaster.close()
    progress.destroy()
    messagebox.showinfo('Mission accomplished', 
            'Verifying finished !\nErrors: ' + str(errors))


# ===================================================================================
# Load Files from Disk
# ===================================================================================

def loadFiles():

    def readFile(fileindex):
        # Create output file
        filename  = cleanstring(directory.filelist[fileindex]['name']) + '.prg'
        blocksize = directory.filelist[fileindex]['size']
        try:
            f = open(folder + '/' + filename, 'wb')
        except:
            messagebox.showerror('Error', 'Failed to create ' + filename + ' !')
            return 1

        # Start read operation
        progress.setactivity('Transfering \"' + directory.filelist[fileindex]['name'] 
                             + '\"\nto \"' + filename + '\" ...')
        progress.setvalue(0)
        track  = directory.filelist[fileindex]['track']
        sector = directory.filelist[fileindex]['sector']
        if dumpmaster.startfastload(track, sector) > 0:
            f.close()
            messagebox.showerror('Error', 'Failed to start disk operation !')
            return 1

        # Read file from disk to output file
        written = 0;
        dumpmaster.timeout = 4
        while 1:
            block = dumpmaster.getblock(256)
            dumpmaster.timeout = 1
            if not block:
                f.close()
                messagebox.showerror('Error', 'Failed to read from disk !')
                return 1
            written += 1
            progress.setvalue(written * 100 // blocksize)
            if block[0] == 0:
                f.write(block[2:block[1]+1])
                break
            f.write(block[2:])

        f.close()
        return 0


    # Establish serial connection
    dumpmaster = Adapter()
    if not dumpmaster.is_open:
        messagebox.showerror('Error', 'DumpMaster64 Adapter not found !')
        return

    # Check if IEC device ist present
    if not dumpmaster.checkdevice(device.get()):
        dumpmaster.close()
        messagebox.showerror('Error', 'IEC device ' + str(device.get()) + ' not found !')
        return

    # Upload fast loader to disk drive RAM
    if dumpmaster.uploadbin(FASTLOAD_LOADADDR, FASTLOAD_BIN) > 0:
        dumpmaster.close()
        messagebox.showerror('Error', 'Failed to upload fastload.bin !')
        return

    # Read directory
    blocks = bytes()
    if dumpmaster.startfastload(18, 0) > 0:
        dumpmaster.close()
        messagebox.showerror('Error', 'Failed to start disk operation !')
        return

    dumpmaster.timeout = 4
    while 1:
        block = dumpmaster.getblock(256)
        dumpmaster.timeout = 1
        if not block:
            dumpmaster.close()
            messagebox.showerror('Error', 'Failed to read directory !')
            return
        blocks += block
        if block[0] == 0:
            break

    directory = Dir(blocks)

    # Draw content window
    contentWindow = Toplevel(mainWindow)
    contentWindow.title('Copy PRG files from disk')
    contentWindow.minsize(200, 100)
    contentWindow.resizable(width=False, height=False)
    contentWindow.transient(mainWindow)
    contentWindow.grab_set()

    actionFrame = Frame(contentWindow, borderwidth = 0)
    l = Listbox(actionFrame, font = 'TkFixedFont', height = 16, width = 30,
                             selectmode='extended')
    l.pack(side='left', fill=BOTH)
    s = Scrollbar(actionFrame, orient = VERTICAL, command = l.yview)
    l['yscrollcommand'] = s.set
    s.pack(side='right', fill='y')
    actionFrame.pack()
    Button(contentWindow, text='Copy selected file(s)', 
            command=contentWindow.quit).pack(pady = 10)

    # Print files
    indices = list()
    index = 0
    for file in directory.filelist:
        if file['type'] == 'PRG' and file['size'] > 0:
            line  = str(file['size']).rjust(4) + '  '
            line += ('"' + file['name'] + '"').ljust(20)
            line += 'PRG'
            l.insert('end', line)
            indices.append(index)
        index += 1

    contentWindow.mainloop()
    selected = l.curselection()

    if selected:
        folder = filedialog.askdirectory(title = 'Select output folder')
        if not folder:
            contentWindow.destroy()
            dumpmaster.close()
            return

        progress = Progressbox(mainWindow, 'Reading file from disk', '')
        for i in selected:
            readFile(indices[i])
        progress.destroy()

    contentWindow.destroy()
    dumpmaster.close()


# ===================================================================================
# Tape Read Function
# ===================================================================================

def tapeRead():
    # Establish serial connection
    dumpmaster = Adapter()
    if not dumpmaster.is_open:
        messagebox.showerror('Error', 'DumpMaster64 Adapter not found !')
        return

    # Open output file and write file header
    fileName = filedialog.asksaveasfilename(title = 'Select output file',
                filetypes = (("TAP files","*.tap"), ('All files','*.*')))

    if not fileName:
        return

    try:
        f = open(fileName, 'wb')
    except:
        messagebox.showerror('Error', 'Could not create output file !')
        dumpmaster.close()
        return

    f.write(b'C64-TAPE-RAW\x01\x00\x00\x00\x00\x00\x00\x00')

    # Create information window
    contentWindow = Toplevel(mainWindow)
    contentWindow.title('Reading from tape')
    contentWindow.resizable(width=False, height=False)
    contentWindow.transient(mainWindow)
    contentWindow.grab_set()

    canvas = Canvas(contentWindow, width=256, height=172)
    canvas.pack()
    canvas.create_rectangle(0, 0, 256, 128, fill="white")

    # Send read command to DumpMaster64 and wait for PLAY pressed
    text1 = canvas.create_text(128, 150, text='PRESS PLAY ON TAPE', fill='black', anchor='c', font=('Helvetica', 12, 'bold'))
    contentWindow.update()
    dumpmaster.sendcommand(CMD_READTAPE)
    while 1:
        response = dumpmaster.read(1)
        if response:
            break

    if response[0] > 0:
        f.close()
        dumpmaster.close()
        contentWindow.destroy()
        messagebox.showerror('Timeout', 'Didn\'t I say something about pressing PLAY?')
        return

    canvas.delete(text1)
    text1 = canvas.create_text(128, 150, text='SEARCHING', fill='black', anchor='c', font=('Helvetica', 12, 'bold'))
    line1 = canvas.create_line(0, 0, 0, 128, fill='black')
    contentWindow.update()

    # Receive data from DumpMaster64 and write to output file
    count     = 0
    fsize     = 0
    checksum  = 0
    taptime   = 0
    pcount    = 0
    point     = []

    while 1:
        data = dumpmaster.read(2)
        if data:
            dataval = int.from_bytes(data, byteorder='little')
            #dataval = int(dataval * 0.985248 + 0.5)
            if dataval == 0:
                break
            if dataval > 255:
                f.write(b'\x00')
                f.write((dataval * 8).to_bytes(3, byteorder='little'))
                fsize   += 4
            else:
                f.write(dataval.to_bytes(1, byteorder='little'))
                fsize    += 1
                if dataval < 128:
                    x = (count // 32) % 256
                    y = dataval
                    i = pcount % 8000
                    if pcount >= 8000:
                        canvas.delete(point[i])
                        point[i] = canvas.create_line(x, y, x+1, y, fill='black')
                    else:
                        point.append(canvas.create_line(x, y, x+1, y, fill='black'))
                    pcount += 1
            count    += 1
            checksum += data[0]
            checksum += data[1]
            checksum %= 65536
            taptime  += dataval
            if count == 1:
                canvas.delete(text1)
                text1 = canvas.create_text(128, 150, text='READING FROM TAPE', fill='black', anchor='c', font=('Helvetica', 12, 'bold'))
                contentWindow.update()
            if count % 32 == 0:
                x = (count // 32 + 2) % 256
                canvas.delete(line1)
                line1 = canvas.create_line(x, 0, x, 128, fill='black')
                contentWindow.update()
        
    taptime  = taptime * 8 // 1000000 + 1
    f.seek(16)
    f.write(fsize.to_bytes(4, byteorder='little'))
    testsum  = int.from_bytes(dumpmaster.read(2), byteorder='little')
    overflow = dumpmaster.read(1)[0]

    # Close output file and serial connection
    f.close()
    dumpmaster.close()
    contentWindow.destroy()

    # Validate data and checksum, print infos, exit
    if count == 0:
        messagebox.showerror('Timeout', 'No data received !')
    elif overflow > 0:
        messagebox.showerror('Error', 'Buffer overflow occured !')
    elif not checksum == testsum:
        messagebox.showerror('Error', 'Checksum mismatch !')
    else:
        messagebox.showinfo('Mission accomplished', 
                'Dumping successful !\nTAP time: ' + str(taptime//60) + ' min ' + str(taptime%60) + ' sec')


# ===================================================================================
# Tape Write Function
# ===================================================================================

def tapeWrite():
    # Establish serial connection
    dumpmaster = Adapter()
    if not dumpmaster.is_open:
        messagebox.showerror('Error', 'DumpMaster64 Adapter not found !')
        return

    # Open input file
    fileName = filedialog.askopenfilename(title = 'Select input file',
                filetypes = (("TAP files","*.tap"), ('All files','*.*')))

    if not fileName:
        return

    try:
        fileSize = os.stat(fileName).st_size
        f = open(fileName, 'rb')
    except:
        messagebox.showerror('Error', 'Could not open input file !')
        dumpmaster.close()
        return

    # Check file header
    if fileSize < 20 or not f.read(12) == b'C64-TAPE-RAW':
        messagebox.showerror('Error', 'Wrong file header !')
        dumpmaster.close()
        f.close()
        return

    # Check TAP version
    tapversion = f.read(1)[0]
    if tapversion > 1:
        messagebox.showerror('Error', 'Unsupported TAP version !')
        dumpmaster.close()
        f.close()
        return

    # Check size of data area
    f.seek(16)
    datasize = int.from_bytes(f.read(4), byteorder='little')
    if not (datasize + 20) == fileSize:
        messagebox.showerror('Error', 'File size does not match header entry !')
        dumpmaster.close()
        f.close()
        return

    # Preparing data and store it in temp file
    try:
        t = open('dumpmaster.tmp', 'wb')
    except:
        messagebox.showerror('Error', 'Could not create temp file !')
        dumpmaster.close()
        f.close()
        return

    fcount  = datasize
    tcount  = 0
    taptime = 0
    progress = Progressbox(mainWindow, 'Writing to tape', 'Preparing data ...')

    while fcount > 0:        
        dataval = f.read(1)[0]
        fcount -= 1
        if dataval > 0:
            #dataval = int(dataval / 0.985248 + 0.5)
            t.write(dataval.to_bytes(2, byteorder='little'))
            tcount  += 2
            taptime += dataval
        else: 
            if tapversion == 1:
                dataval  = int.from_bytes(f.read(3), byteorder='little') // 8
                #dataval  = int(dataval / 0.985248 + 0.5)
                taptime += dataval
                fcount  -= 3
                while dataval > 0:
                    datalittle = dataval % 32640
                    if(datalittle == 0):
                        datalittle = 32640
                    t.write(datalittle.to_bytes(2, byteorder='little'))
                    tcount  += 2
                    dataval -= datalittle
            else:
                dataval = 32640
                t.write(dataval.to_bytes(2, byteorder='little'))
                tcount  += 2
                taptime += dataval
        progress.setvalue((datasize - fcount) * 100 // datasize)

    f.close()
    t.close()
    datasize = tcount
    taptime  = taptime * 8 // 1000000 + 1

    # Send write command to DumpMaster64 and wait for RECORD pressed
    progress.setvalue(0)
    progress.setactivity('Recording time: ' + str(taptime//60) + ' min ' + str(taptime%60) + ' sec\n\nPRESS RECORD & PLAY ON TAPE')
    dumpmaster.sendcommand(CMD_WRITETAPE)

    count = 0
    while 1:
        response = dumpmaster.read(1)
        if response:
            break
        count += 10
        progress.setvalue(count)

    if response[0] > 0:
        f.close()
        dumpmaster.close()
        progress.destroy()
        messagebox.showerror('Timeout', 'Didn\'t I say something about pressing RECORD?')
        return
  
    # Read data from temp file and write to tape
    progress.setvalue(0)
    progress.setactivity('Recording in progress ...')
    t = open('dumpmaster.tmp', 'rb')
    checksum = 0

    while 1:
        response = dumpmaster.read(1)
        if response:
            packsize = response[0]
            if packsize == 0:
                break
            if tcount <= 0:
                dumpmaster.write(b'\x00\x00')
            while packsize > 0 and tcount > 0:
                data = t.read(2)
                tcount -= 2
                dumpmaster.write(data)
                packsize -= 2
                checksum += data[0]
                checksum += data[1]
                if packsize > 0 and tcount <= 0:
                    dumpmaster.write(b'\x00\x00')
                checksum %= 65536
            progress.setvalue((datasize - tcount) * 100 // datasize)

    testsum  = int.from_bytes(dumpmaster.read(2), byteorder='little')
    underrun = dumpmaster.read(1)[0]
    stopped  = dumpmaster.read(1)[0]

    # Close temp file and serial connection
    t.close()
    dumpmaster.close()
    progress.destroy()

    # Validate data and checksum and print infos
    if underrun > 0:
        messagebox.showerror('Error', 'Buffer underrun occured !')

    elif tcount > 0 or stopped > 0:
        messagebox.showerror('Error', 'Recording was stopped before completion !')

    elif not checksum == testsum:
        messagebox.showerror('Error', 'Checksum mismatch !')

    else:
        messagebox.showinfo('Mission accomplished', 
                'Recording successful !\nPRESS STOP ON TAPE')

    os.remove('dumpmaster.tmp')


# ===================================================================================
# Show File Content (HEX View)
# ===================================================================================

def showContent():
    fileName = filedialog.askopenfilename(title = 'Select file for HEX view',
                filetypes = (("D64 files","*.d64"), ("TAP files","*.tap"), 
                            ("BIN files","*.bin"), ('All files','*.*')))
    if not fileName:
        return

    try:
        f = open(fileName, 'rb')
    except:
        messagebox.showerror('Error', 'Failed to open file !')
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
# Flash Firmware
# ===================================================================================

def flashFirmware():
    # Valid target MCUs
    FIRMWARE_TARGETS = {
        0x1E9325: 'attiny804',
        0x1E9322: 'attiny814',
        0x1E9425: 'attiny1604',
        0x1E9422: 'attiny1614'  }

    # Fuse settings
    FIRMWARE_FUSES = {0:0x00, 1:0x00, 2:0x01, 4:0x00, 5:0xC5, 6:0x04, 7:0x00, 8:0x00}

    # Show info
    messagebox.showinfo('Flash Firmware', 
            'Set the serial mode switch to "UPDI" and klick "OK"!')

    # Establish serial connection
    progress = Progressbox(mainWindow, 'Flashing firmware', 'Connecting to device ...')
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
        if not devid in FIRMWARE_TARGETS:
            raise Exception()
    except:
        tinyupdi.leave_progmode()
        tinyupdi.close()
        progress.destroy()
        messagebox.showerror('Error', 'Unknown or unsupported device!')
        return

    # Flash firmware
    progress.setactivity('Flashing and verifying firmware ...')
    progress.setvalue(60)
    try:
        if not tinyupdi.flash_bin(FIRMWARE_BIN):
            raise Exception()
    except:
        tinyupdi.leave_progmode()
        tinyupdi.close()
        progress.destroy()
        messagebox.showerror('Error', 'Flashing firmware failed!')
        return

    # Burning fuses
    progress.setactivity('Writing and verifying fuses ...')
    progress.setvalue(90)
    try:
        for f in FIRMWARE_FUSES:
            if not tinyupdi.set_fuse(f, FIRMWARE_FUSES[f]):
                Exception()
    except:
        tinyupdi.leave_progmode()
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
mainWindow.title('DumpMaster64 v1.3')
mainWindow.resizable(width=False, height=False)

f_demag_var  = IntVar()
f_bump_var   = IntVar()
f_extend_var = IntVar()
f_verify_var = IntVar()
r_bam_var    = IntVar()
r_verify_var = IntVar()
w_bam_var    = IntVar()
w_verify_var = IntVar()
f_bump_var.set(1)
r_bam_var.set(1)
w_bam_var.set(1)

device = IntVar()
device.set(8)
devices = [('#8',8), ('#9',9), ('#10',10), ('#11',11)]

actionFrame = Frame(mainWindow, borderwidth = 2, relief = 'groove')
Label(actionFrame, text = 'Floppy Disk Functions:').pack(pady = 5)
deviceFrame = Frame(actionFrame, borderwidth = 0)
for txt, val in devices:
    Radiobutton(deviceFrame, text=txt, variable=device, value=val).grid(row=1,column=val-8,padx=5)
deviceFrame.pack(pady = 5)
Button(actionFrame, text = 'Format disk', command = diskFormat
            ).pack(padx = 10, pady = 2, fill = 'x')
Button(actionFrame, text = 'Copy disk to D64 file', command = diskRead,
            ).pack(padx = 10, pady = 2, fill = 'x')
Button(actionFrame, text = 'Copy D64 file to disk', command = diskWrite,
            ).pack(padx = 10, pady = 2, fill = 'x')
Button(actionFrame, text = 'Copy PRG files from disk', command = loadFiles,
            ).pack(padx = 10, pady = 2, fill = 'x')
Button(actionFrame, text = 'Show disk directory', command = diskDir,
            ).pack(padx = 10, pady = 2, fill = 'x')
actionFrame.pack(padx =10, pady = 10, ipadx = 5, ipady = 5, fill = 'x')

tapeFrame = Frame(mainWindow, borderwidth = 2, relief = 'groove')
Label(tapeFrame, text = 'Tape Functions:').pack(pady = 5)
Button(tapeFrame, text = 'Read from tape to TAP file', command = tapeRead).pack(padx = 10, pady = 2, fill = 'x')
Button(tapeFrame, text = 'Write TAP file to tape', command = tapeWrite).pack(padx = 10, pady = 2, fill = 'x')
tapeFrame.pack(padx = 10, ipadx = 5, ipady = 5, fill = 'x')

specialFrame = Frame(mainWindow, borderwidth = 2, relief = 'groove')
Label(specialFrame, text = 'Additional Functions:').pack(pady = 5)
Button(specialFrame, text = 'Show file content (hex)', command = showContent,
            ).pack(padx = 10, pady = 2, fill = 'x')
Button(specialFrame, text = 'Flash firmware', command = flashFirmware,
            ).pack(padx = 10, pady = 2, fill = 'x')
specialFrame.pack(padx =10, pady = 10, ipadx = 5, ipady = 5, fill = 'x')

Button(mainWindow, text = 'Exit', command = mainWindow.quit).pack(pady = 10)

mainWindow.mainloop()
