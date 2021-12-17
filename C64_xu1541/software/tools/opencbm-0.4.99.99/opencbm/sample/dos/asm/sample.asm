;/*
; *  This program is free software; you can redistribute it and/or
; *  modify it under the terms of the GNU General Public License
; *  as published by the Free Software Foundation; either version
; *  2 of the License, or (at your option) any later version.
; *
; *  Copyright 2004 Spiro Trikaliotis
; */

	name    dossample
	title   'DOSSAMPLE - Sample for calling the opencbm VDD'

_TEXT   segment byte public 'CODE'

	assume  cs:_TEXT,ds:_TEXT,es:NOTHING

	org     100h

; the following macros (RegisterModule, UnRegisterModule,
; DispatchCall) are taken from the DDK's isvbop.inc file

RegisterModule macro
	db 0c4h, 0c4h, 58h, 0
endm

UnRegisterModule macro
	db 0c4h, 0c4h, 58h, 1
endm

DispatchCall macro
	db 0c4h, 0c4h, 58h, 2
endm

vdd_call1 macro A

    mov dl,A
	mov ax,[hVDD]       ; VDD handle
    DispatchCall

endm

vdd_call macro A

    mov bx,[cbmfile]
    vdd_call1 A

endm


vdd_driver_open macro TEXT

	xor cx,cx

    vdd_call1 0,TEXT

	mov [cbmfile],bx

endm


vdd_driver_close macro TEXT

    vdd_call 1,TEXT

endm

vdd_init macro

    jmp short @@SkipStrings

DllName   DB    "OpenCbmVDD.DLL",0
InitFunc  DB    "VDDRegisterInit",0
DispFunc  DB    "VDDDispatch",0
hVDD      DW    ?
cbmfile   DW    ?

@@SkipStrings: 
    push ds
    pop es
    ; Load opencbmvdd.dll
    mov si, offset DllName                   ; ds:si = dll name
    mov di, offset InitFunc                  ; es:di = init routine
    mov bx, offset DispFunc                  ; ds:bx = dispatch routine

    RegisterModule
    mov [hVDD],AX  ; remember the handle for later calls

endm


vdd_uninit macro

    mov AX,[hVDD]
    UnRegisterModule

endm

Error Macro NR,TEXT

    jc ErrorText&NR
    jnc short SkipText&NR

String&NR: DB '&TEXT',13,10,'$'

ErrorText&NR:
    lea dx,[String&NR]
    Mov ah,9
    int 21h
    jmp quit

SkipText&NR:

endm
    

Start proc near

    vdd_init
    Error 1,<RegisterModule failed>

    vdd_driver_open
    Error 2,<vdd_driver_open failed>

    vdd_driver_close
    Error 3,<vdd_driver_close failed>

Quit:
    vdd_uninit

    mov ah,4ch
    int 21h

PrintError:
    mov ah,9
    int 21h
    jmp Quit

Start endp

_TEXT ends

      end Start
