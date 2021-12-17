/*
  AVRUSBBoot - USB bootloader for Atmel AVR controllers

  Thomas Fischl <tfischl@gmx.de>

  License:
  The project is built with AVR USB driver by Objective Development, which is
  published under a proprietary Open Source license. To conform with this
  license, USBasp is distributed under the same license conditions. See
  documentation.

  Target.........: ATMega8 at 12 MHz
  Creation Date..: 2006-03-18
  Last change....: 2006-06-25

  To adapt the bootloader to your hardware, you have to modify the following files:
  - bootloaderconfig.h:
    Define the condition when the bootloader should be started
  - usbconfig.h:
    Define the used data line pins. You have to adapt USB_CFG_IOPORT, USB_CFG_DMINUS_BIT and
    USB_CFG_DPLUS_BIT to your hardware. The rest should be left unchanged.
*/

#define F_CPU 12000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/boot.h>
#include <util/delay.h>

#include "flasher.h"

/*
#include "xu1541bios.h"
#include "version.h"
*/

void spm_copy(uint8_t what, uint16_t address, uint16_t data)
{
        __asm__ __volatile__
        (
                "movw  r0, %3\n\t"
                "movw r30, %2\n\t"
                "sts %0, %1\n\t"
                "spm\n\t"
                "clr  r1\n\t"
                :
                : "i" (_SFR_MEM_ADDR(__SPM_REG)),
                  "r" (what),
                  "r" ((uint16_t)address),
                  "r" ((uint16_t)data)
                : "r0", "r30", "r31"
        );

        if (what != __BOOT_PAGE_FILL) {
                boot_spm_busy_wait();      // Wait until the memory is written.
                boot_rww_enable();
        }
}

void spm_end(void)
{
}
