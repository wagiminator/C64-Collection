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

#include "xu1541_types.h"

#ifndef USBTINY
// use avrusb library
#include "usbdrv.h"
typedef uint8_t byte_t; /* @@@ */
#else
// use usbtiny library
#include "usb.h"
#include "usbtiny.h"
typedef byte_t uchar;

#define USBDDR DDRC
#define USB_CFG_IOPORT PORTC

#define USB_CFG_DMINUS_BIT USBTINY_DMINUS
#define USB_CFG_DPLUS_BIT USBTINY_DPLUS

#define usbInit()  usb_init()
#define usbPoll()  usb_poll()
#endif

#define USBBOOT_FUNC_WRITE_PAGE 2
#define USBBOOT_FUNC_LEAVE_BOOT 1
#define USBBOOT_FUNC_GET_PAGESIZE 3

#define STATE_IDLE 0
#define STATE_WRITE_PAGE 1

#include "xu1541bios.h"
#include "version.h"

extern unsigned int bios_magic;

#define BOOTLOADER_NO_APPLICATION_MAGIC 0xF347
#define BOOTLOADER_START_FLASHER_MAGIC 0x8123
#define BOOTLOADER_START_APPLICATION_MAGIC 0xBCDE

static uchar state = STATE_IDLE;
static uint16_t page_address;
static uint8_t page_offset;
static uint8_t use_firmware;

xu1541_firmware_data_t firmware_data;

#define MOVESECTION __attribute__ ((section (".textadd")))
// #define SECTIONFLASH __attribute__ ((section (".textflash")))

void start_flash_bootloader(void) MOVESECTION;
void leaveBootloader() MOVESECTION;

static void bios_reboot(void)
{
  wdt_enable(1);
  while (1) {
  }
}

void spm(uint8_t what, uint16_t address, uint16_t data);
// void spm(uint8_t what, uint16_t address, uint16_t data) SECTIONFLASH;

#define jump_to_app() \
        ((void(*)(void))0)()

void leaveBootloader() {
      cli();
      boot_rww_enable();
#if 0
      GICR = (1 << IVCE);  /* enable change of interrupt vectors */
      GICR = (0 << IVSEL); /* move interrupts to application flash section */
      jump_to_app();
#endif

      /* we now have a fully working firmware */
      use_firmware = 1;
}

#ifndef USBTINY
uchar   usbFunctionSetup(uchar data[8]) {
  static uchar replyBuf[8];
  usbMsgPtr = replyBuf;
#else
byte_t  usb_setup ( byte_t data[8] ) {
  byte_t *replyBuf = data;
#endif
  uchar len = 0;

  if (use_firmware) {
    return firmware_usbFunctionSetup(data, replyBuf);
  }

  if (data[1] == XU1541_INFO) {
    replyBuf[0] = 0xff;
    replyBuf[1] = 0xff;
    *(uint16_t*)(replyBuf+2) = XU1541_CAP_BOOTLOADER; /* XU1541_CAPABILIIES; */
    replyBuf[4] = XU1541_BIOS_VERSION_MAJOR;
    replyBuf[5] = XU1541_BIOS_VERSION_MINOR;
    return 6;
  } else
  if (data[1] == USBBOOT_FUNC_LEAVE_BOOT) {
    bios_magic = BOOTLOADER_START_APPLICATION_MAGIC;

    bios_reboot();
  } else if (data[1] == USBBOOT_FUNC_WRITE_PAGE) {
    state = STATE_WRITE_PAGE;

    page_address = (data[3] << 8) | data[2]; /* page address */
    page_offset = 0;

    cli();
    boot_page_erase(page_address); /* erase page */
    sei();
    boot_spm_busy_wait(); /* wait until page is erased */

#ifdef USBTINY
    /* usbtiny always returns 0 on write */
    len = 0;
#else
    len = 0xff; /* multiple out */
#endif

  } else if (data[1] == USBBOOT_FUNC_GET_PAGESIZE) {
    replyBuf[0] = SPM_PAGESIZE >> 8;
    replyBuf[1] = SPM_PAGESIZE & 0xff;
    return 2;

  }
  return len;
}

/*---------------------------------------------------------------------------*/
/* usbFunctionRead                                                           */
/*---------------------------------------------------------------------------*/

#ifndef USBTINY
uchar usbFunctionRead( uchar *data, uchar len )
#else
byte_t usb_in ( byte_t* data, byte_t len )
#endif
        MOVESECTION;

#ifndef USBTINY
uchar usbFunctionRead( uchar *data, uchar len )
#else
byte_t usb_in ( byte_t* data, byte_t len )
#endif
{
  return use_firmware ? firmware_usbFunctionRead(data, len) : 0;
}

#ifndef USBTINY
uchar usbFunctionWrite( uchar *data, uchar len )
#else
void usb_out ( byte_t* data, byte_t len )
#endif
{
  uchar i;

  if (use_firmware) {
#ifndef USBTINY
    return firmware_usbFunctionWrite(data, len);
#else
    firmware_usbFunctionWrite(data, len);
    return;
#endif
  }

  /* check if we are in correct state */
  if (state != STATE_WRITE_PAGE)
#ifndef USBTINY
    return 0xff;
#else
    return;
#endif

  for (i = 0; i < len; i+=2) {

    cli();
    boot_page_fill(page_address + page_offset, data[i] | (data[i + 1] << 8));
    sei();
    page_offset += 2;

    /* check if we are at the end of a page */
    if (page_offset >= SPM_PAGESIZE) {

      /* write page */
      cli();
      boot_page_write(page_address);
      sei();
      boot_spm_busy_wait();

      state = STATE_IDLE;
#ifndef USBTINY
      return 1;
#else
      return;
#endif
    }

  }

#ifndef USBTINY
  return 0;
#endif
}

int main(void) MOVESECTION;
void main_poll(void);

int main(void)
{
    /* check if portb.4 (miso) is tied to gnd and call main application if not */
    PORTB |= _BV(4);    // drive pin high
    DDRB  &=  ~_BV(4);  // pin is input (with pullup)

    use_firmware = 0;

    if (pgm_read_word_near(0) == 0xffff) {
        bios_magic = BOOTLOADER_NO_APPLICATION_MAGIC;
    }

    switch (bios_magic) {
      case BOOTLOADER_START_APPLICATION_MAGIC:
      default:
        // check if pin goes high
        if(PINB & _BV(4)) {
          leaveBootloader();
        }

      case BOOTLOADER_NO_APPLICATION_MAGIC:
        /* FALL THROUGH */

      case BOOTLOADER_START_FLASHER_MAGIC:
        break;
    }

    /* make led output and switch it on */
    DDRD  |=  _BV(1);
    PORTD &= ~_BV(1);

    /* initialize the core firmware before the USB stack */
    if (use_firmware) {
      firmware_init();
    }

    /* clear usb ports */
    USB_CFG_IOPORT   &= (uchar)~((1<<USB_CFG_DMINUS_BIT)|(1<<USB_CFG_DPLUS_BIT));

    /* make usb data lines outputs */
    USBDDR    |= ((1<<USB_CFG_DMINUS_BIT)|(1<<USB_CFG_DPLUS_BIT));

    /* USB Reset by device only required on Watchdog Reset */
    _delay_ms(10);

    /* make usb data lines inputs */
    USBDDR &= ~((1<<USB_CFG_DMINUS_BIT)|(1<<USB_CFG_DPLUS_BIT));

    GICR = (1 << IVCE);  /* enable change of interrupt vectors */
    GICR = (1 << IVSEL); /* move interrupts to boot flash section */

    usbInit();
    sei();

    main_poll();

    return 0;
}

void main_poll(void)
{
  for(;;) {	/* main event loop */
    extern byte_t usb_idle(void);
    wdt_reset();
    usbPoll();

    /* do async iec processing */
    if(use_firmware && usb_idle())
      firmware_handle_idle();
  }

}

void start_flash_bootloader(void)
{
    bios_magic = BOOTLOADER_START_FLASHER_MAGIC;

    /* reboot the AVR */
    bios_reboot();
}

void spm(uint8_t what, uint16_t address, uint16_t data)
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
