/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2005-2007 Spiro Trikaliotis
 *
 */

/*! **************************************************************
** \file sys/include/WINDOWS/arch_cbm_driver.h \n
** \author Spiro Trikaliotis \n
** \n
** \brief Windows-specific definitions for the opencbm driver
**
****************************************************************/

/* Include any configuration definitions */
#include "config.h"

/*! make sure that ECR_OFFSET and the like use 0x02, and not 0x0402
    in parallel.h */
#define DVRH_USE_PARPORT_ECP_ADDR 1

#include <parallel.h>

/*! Name of the executable to be debugged */
#define DBG_PROGNAME "OPENCBM.SYS"

#include "cbmioctl.h"
#include "memtags.h"
#include "util.h"
#include "queue.h"
#include "cbmlog.h"

/*! The name if the driver. This name gets a number appended, starting with 0. */
#define CBMDEVICENAME L"\\DosDevices\\opencbm"

/*! The device extension for the device */
typedef
struct _ARCH_DEVICE_EXTENSION {

    /* Windows book keeping: */

    /*! Pointer to the functional device object */
    PDEVICE_OBJECT Fdo;

    /*! Are we running on an NT4 system? */
    BOOLEAN IsNT4;

    /*! != 0 if ECP/EPP modes are not handled by parport.sys, but by us.
     *  This is only allowed if we are running on NT4.
     *  \remark The NT4 code uses this internally to remember the type
     *  of parallel port (ECP, EPP, SPP, bidir).
     */
    ULONG HandleEcpEppMyself;

    /*! Pointer to the lower device object *
     * \todo Only for WDM driver PDEVICE_OBJECT LowerDeviceObject;
     */

    /*! The name of this device */
    UNICODE_STRING DeviceName;

    /*! QUEUE structure which allows us queueing our IRPs */
    QUEUE IrpQueue;

    /*! The FILE_OBJECT of the parallel port driver */
    PFILE_OBJECT ParallelPortFileObject;

    /*! The DEVICE_OBJECT of the parallel port driver */
    PDEVICE_OBJECT ParallelPortFdo;

    /*! Information about the port we are connected to */
    PPARALLEL_PORT_INFORMATION PortInfo;

    /*! Info for the ISR routine */
    PARALLEL_INTERRUPT_SERVICE_ROUTINE Pisr;

    /*! Return value after connection the ISR */
    PARALLEL_INTERRUPT_INFORMATION Pii;

    /*! This even is used to wake-up the task inside of
        wait_for_listener() again */
    KEVENT EventWaitForListener;

    /*! FLAG: Is the parallel port currently locked? */
    BOOLEAN ParallelPortIsLocked;

    /*! FLAG: Lock the parallel port, regardless if the driver is uninstalled. */
    BOOLEAN ParallelPortLock;

    /*! FLAG: We already allocated the parallel port */
    BOOLEAN ParallelPortAllocated;

    /*! FLAG: The mode of the parallel port has already been set */
    BOOLEAN ParallelPortModeSet;

    /*! FLAG: The interrupt for the parallel port has been allocated */
    BOOLEAN ParallelPortAllocatedInterrupt;

    /*! FLAG: We are not allowed to release and init the bus on initialization or deinitialization,
     *        because a TALK without a corresponding UNTALK has been issued.
     */
    BOOLEAN DoNotReleaseBus;

    /*! The thread handle for the worker thread*/
    HANDLE ThreadHandle;

    /*! Pointer to the thread structure */
    PKTHREAD Thread;

    /*! Boolean value: Should the running thread quit itself? */
    BOOLEAN QuitThread;

    /*! Helper for cbmiec_block_irq */
    KIRQL IecBlockIrqPreviousIrql;

    /*! Helper for cbmiec_block_irq */
#if DBG
    LONG IecBlockIrqUsageCount;
#endif

    /*! Countdown for the initialization of the parallel port on startup */
    LONG CableInitTimer;

} ARCH_DEVICE_EXTENSION;

extern VOID
DriverUnload(IN PDRIVER_OBJECT DriverObject);

extern NTSTATUS
DriverCommonInit(IN PDRIVER_OBJECT Driverobject, IN PUNICODE_STRING RegistryPath);

extern VOID
DriverCommonUninit(VOID);

extern NTSTATUS
AddDeviceCommonInit(IN PDEVICE_OBJECT Fdo, IN PUNICODE_STRING DeviceName, IN PCWSTR ParallelPortName);

extern NTSTATUS
cbm_install(IN PDEVICE_EXTENSION Pdx, OUT PCBMT_I_INSTALL_OUT ReturnBuffer, IN OUT PULONG ReturnLength);

extern NTSTATUS
cbm_lock(IN PDEVICE_EXTENSION Pdx);

extern NTSTATUS
cbm_unlock(IN PDEVICE_EXTENSION Pdx);

extern NTSTATUS
cbm_lock_parport(IN PDEVICE_EXTENSION Pdx);

extern NTSTATUS
cbm_unlock_parport(IN PDEVICE_EXTENSION Pdx);

#if DBG
extern NTSTATUS
cbm_dbg_readbuffer(IN PDEVICE_EXTENSION Pdx, OUT PCHAR ReturnBuffer, IN OUT PULONG ReturnLength);
#endif // #if DBG

extern VOID
cbm_init_registry(IN PUNICODE_STRING RegistryPath, IN PDEVICE_EXTENSION Pdx);

extern VOID
cbm_initialize_cable_deferred(IN PDEVICE_EXTENSION Pdx);

extern NTSTATUS
cbm_startio(IN PDEVICE_OBJECT Fdo, IN PIRP Irp);

extern NTSTATUS
cbm_createopenclose(IN PDEVICE_OBJECT Fdo, IN PIRP Irp);

extern NTSTATUS
cbm_readwrite(IN PDEVICE_OBJECT Fdo, IN PIRP Irp);

extern NTSTATUS
cbm_execute_readwrite(IN PDEVICE_EXTENSION Pdx, IN PIRP Irp);

extern NTSTATUS
cbm_execute_createopen(IN PDEVICE_EXTENSION Pdx, IN PIRP Irp);

extern NTSTATUS
cbm_cleanup(IN PDEVICE_OBJECT Fdo, IN PIRP Irp);

extern NTSTATUS
cbm_execute_close(IN PDEVICE_EXTENSION Pdx, IN PIRP Irp);

extern NTSTATUS
cbm_devicecontrol(IN PDEVICE_OBJECT Fdo, IN PIRP Irp);

extern NTSTATUS
cbm_execute_devicecontrol(IN PDEVICE_EXTENSION Pdx, IN PIRP Irp);

extern BOOLEAN
cbm_isr(IN PKINTERRUPT Interrupt, IN PVOID Pdx);

extern VOID
cbm_thread(IN PVOID Context);

extern NTSTATUS
cbm_start_thread(IN PDEVICE_EXTENSION Pdx);

extern VOID
cbm_stop_thread(IN PDEVICE_EXTENSION Pdx);

#ifndef PENUMERATE_DEFINED
   /*! opaque enumeration structure. */
   typedef PVOID PENUMERATE;
#endif

extern NTSTATUS
ParPortEnumerateOpen(PENUMERATE *EnumStruct);

extern NTSTATUS
ParPortEnumerate(PENUMERATE EnumStruct, PCWSTR *DriverName);

extern VOID
ParPortEnumerateClose(PENUMERATE EnumStruct);

extern NTSTATUS
ParPortInit(PUNICODE_STRING ParallelPortName, PDEVICE_EXTENSION Pdx);

extern NTSTATUS
ParPortDeinit(PDEVICE_EXTENSION Pdx);

extern NTSTATUS
ParPortAllocate(PDEVICE_EXTENSION Pdx);

extern NTSTATUS
ParPortFree(PDEVICE_EXTENSION Pdx);

extern NTSTATUS
ParPortSetMode(PDEVICE_EXTENSION Pdx);

extern NTSTATUS
ParPortUnsetMode(PDEVICE_EXTENSION Pdx);

extern NTSTATUS
ParPortSetModeWdm(PDEVICE_EXTENSION Pdx);

extern NTSTATUS
ParPortUnsetModeWdm(PDEVICE_EXTENSION Pdx);

extern NTSTATUS
ParPortAllocInterrupt(PDEVICE_EXTENSION Pdx, PKSERVICE_ROUTINE Isr);

extern NTSTATUS
ParPortFreeInterrupt(PDEVICE_EXTENSION Pdx);

extern NTSTATUS
ParPortAllowInterruptIoctl(PDEVICE_EXTENSION Pdx);

extern NTSTATUS
cbm_registry_open_for_read(OUT PHANDLE HandleKey, IN PUNICODE_STRING Path);

extern NTSTATUS
cbm_registry_open_hardwarekey(OUT PHANDLE HandleKey, OUT PDEVICE_OBJECT *Pdo,
                              IN PDEVICE_EXTENSION Pdx);

extern NTSTATUS
cbm_registry_close(IN HANDLE HandleKey);

extern NTSTATUS
cbm_registry_close_hardwarekey(IN HANDLE HandleKey, IN PDEVICE_OBJECT Pdo);

extern NTSTATUS
cbm_registry_read_ulong(IN HANDLE HandleKey, IN PCWSTR KeyName, OUT PULONG Value);

extern NTSTATUS
cbm_registry_write_ulong(IN HANDLE HandleKey, IN PCWSTR KeyName, IN ULONG Value);

extern NTSTATUS
CbmOpenDeviceRegistryKey(IN PDEVICE_OBJECT a, IN ULONG b, IN ACCESS_MASK c, OUT PHANDLE d);

extern ULONG
CbmGetCurrentProcessorNumber(VOID);

extern ULONG
CbmGetNumberProcessors(VOID);

extern VOID
CLI(VOID);

extern VOID
STI(VOID);
