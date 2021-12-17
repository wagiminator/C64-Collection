#ifndef XU1541_BIOS_H
#define XU1541_BIOS_H

#define BIOSTABLE __attribute__ ((section (".textbiostable")))
#define FIRMWARETABLE __attribute__ ((section (".textfirmwaretable")))

typedef unsigned char RJMP[2];

typedef
struct xu1541_bios_data_s {
        unsigned char version_major;
        unsigned char version_minor;
        RJMP start_flash_bootloader;
        RJMP spm;
} xu1541_bios_data_t;

extern xu1541_bios_data_t bios_data BIOSTABLE;

#define bios_start_flash_bootloader() \
        ((void(*)(void))&bios_data.start_flash_bootloader)()

#define bios_spm(_what, _address, _data) \
        ((void(*)(uint8_t what, uint16_t address, uint16_t data))&bios_data.spm)(_what, _address, _data)

typedef
struct xu1541_firmware_data_s {
        unsigned char version_major;
        unsigned char version_minor;
        RJMP init;
        RJMP handleIdle;
        RJMP usbFunctionSetup;
        RJMP usbFunctionRead;
        RJMP usbFunctionWrite;
} xu1541_firmware_data_t;

extern xu1541_firmware_data_t firmware_data FIRMWARETABLE;

#define firmware_init() \
        ((void(*)(void))&firmware_data.init)()

#define firmware_handle_idle() \
        ((void(*)(void))&firmware_data.handleIdle)()

#define firmware_usbFunctionSetup(_data, _replyBuf) \
        ((uchar(*)(uchar data[8], uchar *replyBuf))&firmware_data.usbFunctionSetup)(_data, _replyBuf)

#define firmware_usbFunctionRead(_data, _len) \
        ((uchar(*)(uchar *data, uchar len))&firmware_data.usbFunctionRead)(_data, _len)

#define firmware_usbFunctionWrite(_data, _len) \
        ((uchar(*)(uchar *data, uchar len))&firmware_data.usbFunctionWrite)(_data, _len)

/*
void init(void);
void handle_idle(void);
uchar usbFunctionSetup(uchar data[8], byte_t *replyBuf);
uchar usbFunctionRead( uchar *data, uchar len );
uchar usbFunctionWrite( uchar *data, uchar len );
*/
#endif /* #ifndef XU1541_BIOS_H */
