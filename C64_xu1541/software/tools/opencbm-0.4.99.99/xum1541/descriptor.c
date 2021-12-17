/*
 * USB descriptors for the xum1541
 * Copyright (c) 2009 Nate Lawson <nate@root.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#include <avr/pgmspace.h>
#include <LUFA/Drivers/USB/USB.h>
#include "xum1541.h"

// Convert a string to its Unicode literal
#define UNISTR(x)   L ## x

// Device descriptor
const USB_Descriptor_Device_t PROGMEM DeviceDescriptor =
{
    Header: {
        Size: sizeof(USB_Descriptor_Device_t),
        Type: DTYPE_Device,
    },

    USBSpecification:       VERSION_BCD(01.10),
    Class:                  0xff,
    SubClass:               0x00,
    Protocol:               0x00,

    Endpoint0Size:          XUM_ENDPOINT_0_SIZE,

    VendorID:               XUM1541_VID,
    ProductID:              XUM1541_PID,
    ReleaseNumber:          (MODEL << 8) | XUM1541_VERSION,

    ManufacturerStrIndex:   0x01,
    ProductStrIndex:        0x02,
    SerialNumStrIndex:      0x03,

    NumberOfConfigurations: 1,
};

// Entire config descriptor
typedef struct {
    USB_Descriptor_Configuration_Header_t Config;
    USB_Descriptor_Interface_t            Interface;
    USB_Descriptor_Endpoint_t             DataInEndpoint;
    USB_Descriptor_Endpoint_t             DataOutEndpoint;
} USB_Descriptor_Configuration_t;

const USB_Descriptor_Configuration_t PROGMEM ConfigurationDescriptor =
{
    Config: {
        Header: {
            Size: sizeof(USB_Descriptor_Configuration_Header_t),
            Type: DTYPE_Configuration,
        },

        TotalConfigurationSize: sizeof(USB_Descriptor_Configuration_t),
        TotalInterfaces:        1,
        ConfigurationNumber:    1,
        ConfigurationStrIndex:  NO_DESCRIPTOR,
        ConfigAttributes:       USB_CONFIG_ATTR_BUSPOWERED,
        MaxPowerConsumption:    USB_CONFIG_POWER_MA(100),
    },

    Interface: {
        Header: {
            Size: sizeof(USB_Descriptor_Interface_t),
            Type: DTYPE_Interface,
        },

        InterfaceNumber:   0,
        AlternateSetting:  0,
        TotalEndpoints:    2,
        Class:             0xff,
        SubClass:          0x00,
        Protocol:          0x00,
        InterfaceStrIndex: NO_DESCRIPTOR,
    },

    DataInEndpoint: {
        Header: {
            Size: sizeof(USB_Descriptor_Endpoint_t),
            Type: DTYPE_Endpoint,
        },

        EndpointAddress:  (ENDPOINT_DESCRIPTOR_DIR_IN | XUM_BULK_IN_ENDPOINT),
        Attributes:        EP_TYPE_BULK,
        EndpointSize:      XUM_ENDPOINT_BULK_SIZE,
        PollingIntervalMS: 0x00,
    },

    DataOutEndpoint: {
        Header: {
            Size: sizeof(USB_Descriptor_Endpoint_t),
            Type: DTYPE_Endpoint,
        },

        EndpointAddress:  (ENDPOINT_DESCRIPTOR_DIR_OUT | XUM_BULK_OUT_ENDPOINT),
        Attributes:        EP_TYPE_BULK,
        EndpointSize:      XUM_ENDPOINT_BULK_SIZE,
        PollingIntervalMS: 0x00,
    },
};

const USB_Descriptor_String_t PROGMEM LanguageString = {
    Header:        { Size: USB_STRING_LEN(1), Type: DTYPE_String },
    UnicodeString: { LANGUAGE_ID_ENG },
};

const USB_Descriptor_String_t PROGMEM ManufacturerString = {
    Header:        { Size: USB_STRING_LEN(28), Type: DTYPE_String },
    UnicodeString: L"Nate Lawson and OpenCBM team",
};

// Serial for attaching more than one xum1541.
static USB_Descriptor_String_t SerialNumString = {
    Header:        { Size: USB_STRING_LEN(3), Type: DTYPE_String },
    UnicodeString: L"000",
};

#define PRODSTRING(m)  L"xum1541 floppy adapter (" UNISTR(m) L")"

const USB_Descriptor_String_t PROGMEM ProductString = {
    Header: {
        Size: USB_STRING_LEN(((sizeof(PRODSTRING(MODELNAME)) / 2) - 1)),
        Type: DTYPE_String,
    },
    UnicodeString: PRODSTRING(MODELNAME),
};

uint16_t
CALLBACK_USB_GetDescriptor(const uint16_t wValue, const uint8_t wIndex,
  void **const DescriptorAddress, uint8_t *MemoryAddressSpace)
{
    void*    Address = NULL;
    uint16_t Size    = NO_DESCRIPTOR;
    wchar_t* ucdBuf;

    /* generally assume Flash memory access unless specified otherwise */
    *MemoryAddressSpace = MEMSPACE_FLASH;

    switch (wValue >> 8) {
    case DTYPE_Device:
        Address = (void *)&DeviceDescriptor;
        Size    = sizeof(USB_Descriptor_Device_t);
        break;
    case DTYPE_Configuration:
        Address = (void *)&ConfigurationDescriptor;
        Size    = sizeof(USB_Descriptor_Configuration_t);
        break;
    case DTYPE_String:
        switch (wValue & 0xff) {
        case 0x00:
            Address = (void *)&LanguageString;
            Size    = pgm_read_byte(&LanguageString.Header.Size);
            break;
        case 0x01:
            Address = (void *)&ManufacturerString;
            Size    = pgm_read_byte(&ManufacturerString.Header.Size);
            break;
        case 0x02:
            Address = (void *)&ProductString;
            Size    = pgm_read_byte(&ProductString.Header.Size);
            break;
        case 0x03:
            Size   = ~ eeprom_read_byte( EEPROM_SerialNumber );
            ucdBuf = SerialNumString.UnicodeString + 2;
            /*
                precondition:  SerialNumString.UnicodeString := L"000"
            */
            for( Size &= 0xff ; Size > 0 ; Size /= 10 , --ucdBuf ) {
                *ucdBuf = Size % 10 + 0x30;
            }
            Address = (void *)&SerialNumString;
            Size    = SerialNumString.Header.Size;
            *MemoryAddressSpace = MEMSPACE_RAM;
            break;
        }
        break;
    }

    *DescriptorAddress = Address;
    return Size;
}
