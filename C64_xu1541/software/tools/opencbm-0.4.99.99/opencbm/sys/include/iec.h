/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2004-2009 Spiro Trikaliotis
 *
 */

/*! **************************************************************
** \file sys/include/iec.h \n
** \author Spiro Trikaliotis \n
** \n
** \brief Definitions for the libiec library
**
****************************************************************/

#ifndef CBMIEC_H
#define CBMIEC_H

/*! status of the bus after a call to cbmiec_check_device() */
typedef
enum iec_checkdevice
{
    IEC_CHECKDEVICE_BUSFREE = 0,  /*!< we are free to use the bus */
    IEC_CHECKDEVICE_NODEVICE = 1, /*!< no device attached */
    IEC_CHECKDEVICE_BUSBUSY = 2   /*!< the bus is still busy */
} IEC_CHECKDEVICE;

extern NTSTATUS
cbmiec_wait_for_drives_ready(IN PDEVICE_EXTENSION Pdx);

extern NTSTATUS
cbmiec_reset(IN PDEVICE_EXTENSION Pdx);

extern BOOLEAN
cbmiec_send_byte(IN PDEVICE_EXTENSION Pdx, IN UCHAR Byte);

extern VOID
cbmiec_wait_for_listener(IN PDEVICE_EXTENSION Pdx, IN BOOLEAN SendEoi);

extern VOID
cbmiec_release_bus(IN PDEVICE_EXTENSION Pdx);

extern NTSTATUS
cbmiec_untalk(IN PDEVICE_EXTENSION Pdx);

extern NTSTATUS
cbmiec_unlisten(IN PDEVICE_EXTENSION Pdx);

extern NTSTATUS
cbmiec_listen(IN PDEVICE_EXTENSION Pdx, IN UCHAR DeviceAddress, IN UCHAR SecondaryAddress);

extern NTSTATUS
cbmiec_talk(IN PDEVICE_EXTENSION Pdx, IN UCHAR DeviceAddress, IN UCHAR SecondaryAddress);

extern NTSTATUS
cbmiec_open(IN PDEVICE_EXTENSION Pdx, IN UCHAR DeviceAddress, IN UCHAR SecondaryAddress);

extern NTSTATUS
cbmiec_close(IN PDEVICE_EXTENSION Pdx, IN UCHAR DeviceAddress, IN UCHAR SecondaryAddress);

extern NTSTATUS
cbmiec_get_eoi(IN PDEVICE_EXTENSION Pdx, OUT PBOOLEAN Result);

extern NTSTATUS
cbmiec_clear_eoi(IN PDEVICE_EXTENSION Pdx);

extern NTSTATUS
cbmiec_iec_wait(IN PDEVICE_EXTENSION Pdx, IN UCHAR Line, IN UCHAR State, OUT PUCHAR Result);

extern NTSTATUS
cbmiec_iec_poll(IN PDEVICE_EXTENSION Pdx, OUT PUCHAR Result);

extern NTSTATUS
cbmiec_iec_set(IN PDEVICE_EXTENSION Pdx, IN USHORT Line);

extern NTSTATUS
cbmiec_iec_release(IN PDEVICE_EXTENSION Pdx, IN USHORT Line);

extern NTSTATUS
cbmiec_iec_setrelease(IN PDEVICE_EXTENSION Pdx, IN USHORT Set, IN USHORT Release);

extern NTSTATUS
cbmiec_iec_dbg_read(IN PDEVICE_EXTENSION Pdx, OUT UCHAR * Value);

extern NTSTATUS
cbmiec_iec_dbg_write(IN PDEVICE_EXTENSION Pdx, IN UCHAR Value);

extern NTSTATUS
cbmiec_pp_read(IN PDEVICE_EXTENSION Pdx, OUT UCHAR *Byte);

extern NTSTATUS
cbmiec_pp_write(IN PDEVICE_EXTENSION Pdx, IN UCHAR Byte);

extern BOOLEAN
cbmiec_interrupt(IN PDEVICE_EXTENSION Pdx);

extern NTSTATUS
cbmiec_cleanup(IN PDEVICE_EXTENSION Pdx);

extern NTSTATUS
cbmiec_init(IN PDEVICE_EXTENSION Pdx);

extern NTSTATUS
cbmiec_global_init(IN PHANDLE HKey);

extern NTSTATUS
cbmiec_raw_write(IN PDEVICE_EXTENSION Pdx, IN const PUCHAR Buffer, IN ULONG BufferLength, OUT ULONG* Written);

extern NTSTATUS
cbmiec_raw_read(IN PDEVICE_EXTENSION Pdx, OUT PUCHAR Buffer, IN ULONG BufferLength, OUT ULONG* Read);

extern VOID
cbmiec_set_cabletype(IN PDEVICE_EXTENSION Pdx, IN IEC_CABLETYPE CableType);

extern NTSTATUS
cbmiec_check_device(IN PDEVICE_EXTENSION Pdx, OUT IEC_CHECKDEVICE *CheckDevice);

extern NTSTATUS
cbmiec_parallel_burst_read(IN PDEVICE_EXTENSION Pdx, OUT UCHAR* Byte);

extern NTSTATUS
cbmiec_parallel_burst_write(IN PDEVICE_EXTENSION Pdx, IN UCHAR Byte);

extern NTSTATUS
cbmiec_parallel_burst_read_track(IN PDEVICE_EXTENSION Pdx, OUT UCHAR* Buffer, IN ULONG BufferLength);

extern NTSTATUS
cbmiec_parallel_burst_read_track_var(IN PDEVICE_EXTENSION Pdx, OUT UCHAR* Buffer, IN ULONG BufferLength);

extern NTSTATUS
cbmiec_parallel_burst_write_track(IN PDEVICE_EXTENSION Pdx, IN UCHAR* Buffer, IN ULONG BufferLength);

extern NTSTATUS
cbmiec_test_irq(IN PDEVICE_EXTENSION Pdx, OUT PVOID Buffer, IN ULONG BufferLength);

extern NTSTATUS
cbmiec_checkcable(PDEVICE_EXTENSION Pdx);

extern BOOLEAN
cbmiec_is_cable_state_wrong(PDEVICE_EXTENSION Pdx);

#endif /* #ifndef CBMIEC_H */
