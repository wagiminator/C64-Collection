;; #include "version.h"

	.data
;; nothing

	.text
        .section .textfirmwaretable, "ax"
	.global	firmware_data
	.type	firmware_data, @function

;; This is the BIOS jump table
;;

firmware_data:
	.byte   0 ; XU1541_VERSION_MAJOR
        .byte   0 ; XU1541_VERSION_MINOR
        rjmp    main
	rjmp	dummy
        rjmp    dummy
        rjmp    dummy
        rjmp    dummy
