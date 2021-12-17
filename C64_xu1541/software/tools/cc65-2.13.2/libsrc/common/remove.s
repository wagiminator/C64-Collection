;
; Ullrich von Bassewitz, 2003-08-12
;
; int __fastcall__ remove (const char* name);
;

        .export         _remove

        .import         __sysremove
        .import         oserrcheck


;--------------------------------------------------------------------------

.proc   _remove

        jsr     __sysremove     ; Call the machine specific function
        jmp     oserrcheck      ; Store into _oserror, set errno, return 0/-1

.endproc




