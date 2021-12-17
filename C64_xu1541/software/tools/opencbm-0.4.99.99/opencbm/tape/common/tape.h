/*
 *  CBM 1530/1531 tape routines.
 *  Copyright 2012 Arnd Menge, arnd(at)jonnz(dot)de
*/

// Compatible tape firmware version (check tape_153x.c)
#define TapeFirmwareVersion 0x0001

// Tape status values (must match xum1541 firmware values in xum1541.h)
#define Tape_Status_OK                              1
#define Tape_Status_OK_Tape_Device_Present          (Tape_Status_OK + 1)
#define Tape_Status_OK_Tape_Device_Not_Present      (Tape_Status_OK + 2)
#define Tape_Status_OK_Device_Configured_for_Read   (Tape_Status_OK + 3)
#define Tape_Status_OK_Device_Configured_for_Write  (Tape_Status_OK + 4)
#define Tape_Status_OK_Sense_On_Play                (Tape_Status_OK + 5)
#define Tape_Status_OK_Sense_On_Stop                (Tape_Status_OK + 6)
#define Tape_Status_OK_Motor_On                     (Tape_Status_OK + 7)
#define Tape_Status_OK_Motor_Off                    (Tape_Status_OK + 8)
#define Tape_Status_OK_Capture_Finished             (Tape_Status_OK + 9)
#define Tape_Status_OK_Write_Finished               (Tape_Status_OK + 10)
#define Tape_Status_OK_Config_Uploaded              (Tape_Status_OK + 11)
#define Tape_Status_OK_Config_Downloaded            (Tape_Status_OK + 12)
#define Tape_Status_ERROR                           255
#define Tape_Status_ERROR_Device_Disconnected       (Tape_Status_ERROR - 1)
#define Tape_Status_ERROR_Device_Not_Configured     (Tape_Status_ERROR - 2)
#define Tape_Status_ERROR_Sense_Not_On_Record       (Tape_Status_ERROR - 3)
#define Tape_Status_ERROR_Sense_Not_On_Play         (Tape_Status_ERROR - 4)
#define Tape_Status_ERROR_Write_Interrupted_By_Stop (Tape_Status_ERROR - 5)
#define Tape_Status_ERROR_usbSendByte               (Tape_Status_ERROR - 6)
#define Tape_Status_ERROR_usbRecvByte               (Tape_Status_ERROR - 7)
#define Tape_Status_ERROR_External_Break            (Tape_Status_ERROR - 8)
#define Tape_Status_ERROR_Wrong_Tape_Firmware       (Tape_Status_ERROR - 9) // Not returned by firmware.

// Signal edge definitions.
#define XUM1541_TAP_WRITE_STARTFALLEDGE 0x20 // start writing with falling edge (1 = true)
#define XUM1541_TAP_READ_STARTFALLEDGE  0x40 // start reading with falling edge (1 = true)

// Tape/disk mode error return values for xum1541_ioctl, xum1541_read, xum1541_write.
#define XUM1541_Error_NoTapeSupport      -100
#define XUM1541_Error_NoDiskTapeMode     -101
#define XUM1541_Error_TapeCmdInDiskMode  -102
#define XUM1541_Error_DiskCmdInTapeMode  -103
