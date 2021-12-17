#include "version.h"

	.data
;; nothing

	.text
        .section .textbiostable, "ax"

	.global	bios_data
	.type	bios_data, @function

;; This is the BIOS jump table
;;

bios_data:
	.byte   XU1541_BIOS_VERSION_MAJOR
        .byte   XU1541_BIOS_VERSION_MINOR
        rjmp    start_flash_bootloader
        rjmp    spm

;; allow for using rcall to jump to the bios reset vector:
;;

#if 0
	.section .text
	.global	bios_restart
bios_restart = 0x1800
#endif

;; give the address for the "magic" byte here
;; this allows for a more "elegant" C access to it

	.section .data
	.global bios_magic
bios_magic = 0x1fe
