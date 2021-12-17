#include "version.h"

	.data
;; nothing

	.text
        .section .textfirmwaretable, "ax"
	.global	firmware_data
	.type	firmware_data, @function

;; This is the BIOS jump table
;;

firmware_data:
	.byte   XU1541_VERSION_MAJOR
        .byte   XU1541_VERSION_MINOR
        rjmp    init
	rjmp	xu1541_handle
        rjmp    usbFunctionSetup
        rjmp    usbFunctionRead
        rjmp    usbFunctionWrite
