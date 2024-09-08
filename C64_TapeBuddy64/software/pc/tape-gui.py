#!/usr/bin/env python3
# ===================================================================================
# Project:   TapeBuddy64 - Graphical Front End written in Python
# Version:   v1.2
# Year:      2021
# Author:    Stefan Wagner
# Github:    https://github.com/wagiminator
# License:   http://creativecommons.org/licenses/by-sa/3.0/
# ===================================================================================
#
# Description:
# ------------
# TapeBuddy64 is a simple and inexpensive adapter that can interface a Commodore
# Datasette to your PC via USB in order to read from or write to tapes.
#
# References:
# -----------
# - https://ist.uwaterloo.ca/~schepers/formats/TAP.TXT
# - https://wav-prg.sourceforge.io/tape.html
# - https://github.com/francescovannini/truetape64
#
# Dependencies:
# -------------
# - adapter  (included in libs folder)
# - tinyupdi (included in libs folder)
# - tkinter  (8.6 or newer)
#
# Operating Instructions:
# -----------------------
# - Set the serial mode switch on your TapeBuddy64 to "UART"
# - Connect your TapeBuddy64 to your Commodore Datasette
# - Connect your TapeBuddy64 to a USB port of your PC
# - Execute this skript: python tape-gui.py


import os
import sys
from tkinter import *
from tkinter import messagebox, filedialog
from tkinter.ttk import *
from libs.adapter import *
from libs.tinyupdi import Programmer


# Firmware binary file
FIRMWARE_BIN   = 'libs/firmware.bin'


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
# Show File Content (HEX View)
# ===================================================================================

def showContent():
    fileName = filedialog.askopenfilename(title = 'Select file for HEX view',
                filetypes = (("TAP files","*.tap"), ('All files','*.*')))
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
# Tape Read Function
# ===================================================================================

def tapeRead():
    # Establish serial connection
    tapebuddy = Adapter()
    if not tapebuddy.is_open:
        messagebox.showerror('Error', 'TapeBuddy64 Adapter not found !')
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
        tapebuddy.close()
        return

    f.write(b'C64-TAPE-RAW\x01\x00\x00\x00\x00\x00\x00\x00')


    # Create information window
    contentWindow = Toplevel(mainWindow)
    contentWindow.title('TapeBuddy64 - Reading from tape')
    contentWindow.resizable(width=False, height=False)
    contentWindow.transient(mainWindow)
    contentWindow.grab_set()

    canvas = Canvas(contentWindow, width=256, height=172)
    canvas.pack()
    canvas.create_rectangle(0, 0, 256, 128, fill="white")


    # Send read command to TapeBuddy64 and wait for PLAY pressed
    text1 = canvas.create_text(128, 150, text='PRESS PLAY ON TAPE', fill='black', anchor='c', font=('Helvetica', 12, 'bold'))
    contentWindow.update()
    tapebuddy.sendcommand(CMD_READTAPE)
    while 1:
        response = tapebuddy.read(1)
        if response:
            break

    if response[0] > 0:
        f.close()
        tapebuddy.close()
        contentWindow.destroy()
        messagebox.showerror('Timeout', 'Didn\'t I say something about pressing PLAY?')
        return

    canvas.delete(text1)
    text1 = canvas.create_text(128, 150, text='SEARCHING', fill='black', anchor='c', font=('Helvetica', 12, 'bold'))
    line1 = canvas.create_line(0, 0, 0, 128, fill='black')
    contentWindow.update()

    # Receive data from TapeBuddy64 and write to output file
    count     = 0
    fsize     = 0
    checksum  = 0
    taptime   = 0
    pcount    = 0
    point     = []

    while 1:
        data = tapebuddy.read(2)
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
    testsum  = int.from_bytes(tapebuddy.read(2), byteorder='little')
    overflow = tapebuddy.read(1)[0]


    # Close output file and serial connection
    f.close()
    tapebuddy.close()
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
    tapebuddy = Adapter()
    if not tapebuddy.is_open:
        messagebox.showerror('Error', 'TapeBuddy64 Adapter not found !')
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
        tapebuddy.close()
        return


    # Check file header
    if fileSize < 20 or not f.read(12) == b'C64-TAPE-RAW':
        messagebox.showerror('Error', 'Wrong file header !')
        tapebuddy.close()
        f.close()
        return


    # Check TAP version
    tapversion = f.read(1)[0]
    if tapversion > 1:
        messagebox.showerror('Error', 'Unsupported TAP version !')
        tapebuddy.close()
        f.close()
        return


    # Check size of data area
    f.seek(16)
    datasize = int.from_bytes(f.read(4), byteorder='little')
    if not (datasize + 20) == fileSize:
        messagebox.showerror('Error', 'File size does not match header entry !')
        tapebuddy.close()
        f.close()
        return


    # Preparing data and store it in temp file
    try:
        t = open('tapebuddy.tmp', 'wb')
    except:
        messagebox.showerror('Error', 'Could not create temp file !')
        tapebuddy.close()
        f.close()
        return

    fcount  = datasize
    tcount  = 0
    taptime = 0
    progress = Progressbox(mainWindow, 'TapeBuddy64 - Writing to tape', 'Preparing data ...')

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


    # Send write command to TapeBuddy64 and wait for RECORD pressed
    progress.setvalue(0)
    progress.setactivity('Recording time: ' + str(taptime//60) + ' min ' + str(taptime%60) + ' sec\n\nPRESS RECORD & PLAY ON TAPE')
    tapebuddy.sendcommand(CMD_WRITETAPE)

    count = 0
    while 1:
        response = tapebuddy.read(1)
        if response:
            break
        count += 10
        progress.setvalue(count)

    if response[0] > 0:
        f.close()
        tapebuddy.close()
        progress.destroy()
        messagebox.showerror('Timeout', 'Didn\'t I say something about pressing RECORD?')
        return
  

    # Read data from temp file and write to tape
    progress.setvalue(0)
    progress.setactivity('Recording in progress ...')
    t = open('tapebuddy.tmp', 'rb')
    checksum = 0

    while 1:
        response = tapebuddy.read(1)
        if response:
            packsize = response[0]
            if packsize == 0:
                break
            if tcount <= 0:
                tapebuddy.write(b'\x00\x00')
            while packsize > 0 and tcount > 0:
                data = t.read(2)
                tcount -= 2
                tapebuddy.write(data)
                packsize -= 2
                checksum += data[0]
                checksum += data[1]
                if packsize > 0 and tcount <= 0:
                    tapebuddy.write(b'\x00\x00')
                checksum %= 65536
            progress.setvalue((datasize - tcount) * 100 // datasize)

    testsum  = int.from_bytes(tapebuddy.read(2), byteorder='little')
    underrun = tapebuddy.read(1)[0]
    stopped  = tapebuddy.read(1)[0]


    # Close temp file and serial connection
    t.close()
    tapebuddy.close()
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

    os.remove('tapebuddy.tmp')


# ===================================================================================
# Flash Firmware
# ===================================================================================

def flashFirmware():
    # Valid target MCUs
    FIRMWARE_TARGETS = {
        0x1E9226: 'attiny404',
        0x1E9222: 'attiny414',
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
    progress = Progressbox(mainWindow, 'TapeBuddy64 - Flashing firmware', 'Connecting to device ...')
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
mainWindow.title('TapeBuddy64')
mainWindow.resizable(width=False, height=False)
mainWindow.minsize(200, 100)

tapeFrame = Frame(mainWindow, borderwidth = 2, relief = 'groove')
Label(tapeFrame, text = 'Tape Functions:').pack(pady = 5)
Button(tapeFrame, text = 'Read from tape to TAP file', command = tapeRead).pack(padx = 10, pady = 2, fill = 'x')
Button(tapeFrame, text = 'Write TAP file to tape', command = tapeWrite).pack(padx = 10, pady = 2, fill = 'x')
tapeFrame.pack(padx = 10, pady = 10, ipadx = 5, ipady = 5, fill = 'x')

specialFrame = Frame(mainWindow, borderwidth = 2, relief = 'groove')
Label(specialFrame, text = 'Additional Functions:').pack(pady = 5)
Button(specialFrame, text = 'Show File Content (hex)', command = showContent).pack(padx = 10, pady = 2, fill = 'x')
Button(specialFrame, text = 'Flash firmware', command = flashFirmware).pack(padx = 10, pady = 2, fill = 'x')
specialFrame.pack(padx = 10, pady = 5, ipadx = 5, ipady = 5, fill = 'x')

Button(mainWindow, text = 'Exit', command = mainWindow.quit).pack(pady = 10)

mainWindow.mainloop()
