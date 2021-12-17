/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2004-2008 Spiro Trikaliotis
 */

/*! **************************************************************
** \file include/debug.h \n
** \author Spiro Trikaliotis \n
** \n
** \brief Define makros for debugging purposes
**
****************************************************************/

#ifndef DBG_H
#define DBG_H

/* Make sure at least one of DBG_KERNELMODE or DBG_USERMODE is defined */
#if !defined(DBG_KERNELMODE) && !defined(DBG_USERMODE)
   #error DBG_KERNELMODE or DBG_USERMODE must be specified!
#endif

/* Make sure DBG_KERNELMODE and DBG_USERMODE are not defined simultaneously */
#if defined(DBG_KERNELMODE) && defined(DBG_USERMODE)
   #error Only one of DBG_KERNELMODE and DBG_USERMODE must be specified!
#endif

#ifndef DBG
 #ifdef _DEBUG
  #define DBG 1
  #define __FUNCTION__ ""
 #endif
#endif

/*! \brief Execute a breakpoint

 \remark
   Depending upon the environment, this macro uses different means
   to generate a system agnostic breakpoint.
*/
#if defined(_PREFAST_)
  #define DBG_I_BREAKPOINT() __debugbreak
#elif defined(DBG_KERNELMODE)
  #define DBG_I_BREAKPOINT() DbgBreakPoint()
#else
  #ifdef __WIN32
    #define DBG_I_BREAKPOINT() DebugBreak()
  #else
    #define DBG_I_BREAKPOINT()
  #endif
#endif

#if DBG

#ifndef _WIN32

#include <stdlib.h>
#include <stdio.h>
#define OutputDebugString(_xxx) printf( "%s\n", _xxx);

#define IN const
#define OUT
#define STATUS_SUCCESS 0

typedef void VOID;
typedef char CHAR;
typedef unsigned char UCHAR;
typedef long LONG;
typedef unsigned long ULONG;
typedef char* PCHAR;
typedef unsigned long* PULONG;

typedef int NTSTATUS;
typedef void* PDEVICE_EXTENSION;

#endif

       extern void dbg_memdump(const char *Where,
           const unsigned char *InputBuffer,
           const unsigned int Count);

       /*! \brief output a memory dump to the debugging system

        Generates a byte dump and outputs it to the debugging system

        \param _Where
           Some text to output in front of the memory dump.
           This text can be used to identify where this dump
           occurred.

        \param _Buffer
           Pointer to a buffer which contains the data
           bytes to be dumped.

        \param _Count
           The size of the buffer to be dumped.

        \remark
           as dumping memory is used regularly when debugging,
           we give a debug helper function for this
       */
       #define DBG_MEMDUMP(_Where, _Buffer, _Count) dbg_memdump(_Where, _Buffer, _Count)

        /*! \brief Maximum size of a debugging buffer */
       #define DBG_MAX_BUFFERLEN 4096

#ifdef DBG_KERNELMODE

       /*! \brief the maximum number of buffers to be used for debugging */
       #define DBG_MAX_BUFFER 32 /* This *MUST* be a power of 2! */

#else

       /*! \brief the maximum number of buffers to be used for debugging */
       #define DBG_MAX_BUFFER 1 /* This *MUST* be a power of 2! */

#endif // #ifdef DBG_KERNELMODE

       /* define the various flags for debugging */

       /*! debugging flag: generate breakpoints */
       #define DBGF_BREAK   0x80000000

       /*! debugging flag: show entering of functions */
       #define DBGF_ENTER   0x40000000

       /*! debugging flag: show leaving of functions */
       #define DBGF_LEAVE   0x20000000

       /*! debugging flag: show leaving of functions with return value NTSTATUS,
           if it is not STATUS_SUCCESS */
       #define DBGF_LEAVE_FAILURE 0x10000000

       /*! debugging flag: show parameters of entered functions */
       #define DBGF_PARAM   0x08000000

#ifdef DBG_KERNELMODE

       /*! debugging flag: show IEC related output */
       #define DBGF_IEC     0x04000000

       /*! debugging flag: show IRQ related output */
       #define DBGF_IRQ     0x02000000

       /*! debugging flag: show ASSERTs for IRQL */
       #define DBGF_ASSERTIRQL  0x01000000

       /*! debugging flag: show I/O port accesses */
       #define DBGF_PORT    0x00800000

       /*! debugging flag: show thread related debugging output */
       #define DBGF_THREAD  0x00400000

       /*! debugging flag: show IRP path */
       #define DBGF_IRPPATH 0x00200000

       /*! debugging flag: show IRP processing */
       #define DBGF_IRP     0x00100000

       /*! debugging flag: show DPC processing */
       #define DBGF_DPC     0x00080000

       /*! debugging flag: show CABLE detection */
       #define DBGF_CABLE   0x00040000


       /*! debugging flag: output debugging info debug into Memory Buffer */
       #define DBGF_DBGMEMBUF 0x00020000

       /*! debugging flag: output debugging info via DbgPrint */
       #define DBGF_DBGPRINT 0x00010000

#endif // #ifdef DBG_KERNELMODE

       /*! debugging flag: show parallel port acquisition related output */
       #define DBGF_PPORT   0x0010

       /*! debugging flag: show SUCCESS messages */
       #define DBGF_SUCCESS 0x0008

       /*! debugging flag: show WARNINGS */
       #define DBGF_WARNING 0x0004

       /*! debugging flag: show ERRORS */
       #define DBGF_ERROR   0x0002

       /*! debugging flag: show ASSERTs */
       #define DBGF_ASSERT  0x0001

#ifdef DBG_DLL
/* Make the linker happy if the DLL is to be generated */
int __cdecl main(int argc, char *argv[])
{
}
#endif


/*! hint for the optimiser (and prefast): _x holds true */
#ifdef _WIN32
#define DBG_I_ASSUME(_x) __assume(_x)
#else
#define DBG_I_ASSUME(_x)
#endif

#ifdef DBG_KERNELMODE

       /*! return the name of NTSTATUS values */
       extern const UCHAR *DebugNtStatus(NTSTATUS Value);

#endif // #ifdef DBG_KERNELMODE


       extern unsigned char DbgBuffer[DBG_MAX_BUFFER][DBG_MAX_BUFFERLEN];
       extern          int  DbgBufferPos[];
       extern unsigned long DbgFlags;

       extern void DbgOutputIntoBuffer(unsigned long BufferNumber, const char * const Format, ...);


       /* Some makros for handling the various debug conditions */

       #define ISDBG_BREAK()          (DbgFlags & DBGF_BREAK)                      /*!< is DBGF_BREAK defined? */
       #define ISDBG_ENTER()          (DbgFlags & DBGF_ENTER)                      /*!< is DBGF_ENTER defined? */
       #define ISDBG_LEAVE()          (DbgFlags & DBGF_LEAVE)                      /*!< is DBGF_LEAVE defined? */
       #define ISDBG_LEAVE_FAILURE()  (DbgFlags & (DBGF_LEAVE|DBGF_LEAVE_FAILURE)) /*!< is DBGF_LEAVE or DBGF_LEAVE_FAILURE defined? */
       #define ISDBG_PARAM()          (DbgFlags & DBGF_PARAM)                      /*!< is DBGF_PARAM defined? */
#ifdef DBG_KERNELMODE
       #define ISDBG_IEC()            (DbgFlags & DBGF_IEC)                        /*!< is DBGF_IEC defined? */
       #define ISDBG_IRQ()            (DbgFlags & DBGF_IRQ)                        /*!< is DBGF_IRQ defined? */
       #define ISDBG_ASSERTIRQL()     (DbgFlags & DBGF_ASSERTIRQL)                 /*!< is DBGF_ASSERTIRQL defined? */
       #define ISDBG_PORT()           (DbgFlags & DBGF_PORT)                       /*!< is DBGF_PORT defined? */
       #define ISDBG_THREAD()         (DbgFlags & DBGF_THREAD)                     /*!< is DBGF_THREAD defined? */
       #define ISDBG_IRPPATH()        (DbgFlags & DBGF_IRPPATH)                    /*!< is DBGF_IRPPATH defined? */
       #define ISDBG_IRP()            (DbgFlags & DBGF_IRP)                        /*!< is DBGF_IRP defined? */
       #define ISDBG_DPC()            (DbgFlags & DBGF_DPC)                        /*!< is DBGF_DPC defined? */
       #define ISDBG_CABLE()          (DbgFlags & DBGF_CABLE)                      /*!< is DBGF_CABLE defined? */
       #define ISDBG_DBGMEMBUF()      (DbgFlags & DBGF_DBGMEMBUF)                  /*!< is DBGF_DBGMEMBUF defined? */
       #define ISDBG_DBGPRINT()       (DbgFlags & DBGF_DBGPRINT)                   /*!< is DBGF_DBGPRINT defined? */
#endif // #ifdef DBG_KERNELMODE
       #define ISDBG_PPORT()          (DbgFlags & DBGF_PPORT)                      /*!< is DBGF_PPORT defined? */
       #define ISDBG_SUCCESS()        (DbgFlags & DBGF_SUCCESS)                    /*!< is DBGF_SUCCESS defined? */
       #define ISDBG_WARN()           (DbgFlags & DBGF_WARNING)                    /*!< is DBGF_WARNING defined? */
       #define ISDBG_ERROR()          (DbgFlags & DBGF_ERROR)                      /*!< is DBGF_ERROR defined? */
       #define ISDBG_ASSERT()         (DbgFlags & DBGF_ASSERT)                     /*!< is DBGF_ASSERT defined? */
       #define ISDBG_PANIC()          (1)                                          /*!< is DBGF_PANIC defined? (does not exist, always 1 = yes) */

/* Now, abstract from some differences between user-mode and kernel-mode */

#ifdef DBG_KERNELMODE

              extern VOID DbgInit(VOID);
              extern VOID DbgAllocateMemoryBuffer(VOID);
              extern VOID DbgFreeMemoryBuffer(VOID);
              extern VOID DbgOutputMemoryBuffer(const char *String);

              /*! This macro is called to output the buffer */
              #define _DBG_PERFORM(_xxx) \
                    if (ISDBG_DBGMEMBUF()) \
                    { \
                        DbgOutputMemoryBuffer(_xxx); \
                    } \
                    if (ISDBG_DBGPRINT()) \
                    { \
                        DbgPrint("%s", _xxx); \
                    }

              /*! What has to be defined at the start of each function? */
              #define FUNC_DEF           ULONG DebugBufferNo = 0;

              /*! Some additional work when the debug buffer is initialized.
                \n In kernel-mode, this loop searches a free DebugBuffer to be used. */
#define _DBG_START_ADD  { ULONG BufferUsed; \
       do { \
           DebugBufferNo = InterlockedIncrement(&DebugBufferCounter) & (DBG_MAX_BUFFER-1); \
           BufferUsed = InterlockedExchange(&DebugBufferUsed[DebugBufferNo], 1); \
       } while (BufferUsed);  }

              /*! Some additional work when we are done with the debug buffer.
                \n In kernel-mode, we have to mark the debug buffer as unused. */
#define _DBG_END_ADD InterlockedExchange(&DebugBufferUsed[DebugBufferNo], 0);

              extern LONG DebugBufferCounter;
              extern LONG DebugBufferUsed[];

              /*! Get the number of the debug buffer.
              \n In kernel-mode, the variable DebugBufferNo contains it. */
              #define DEBUG_BUFFER_NO DebugBufferNo

#else // #ifdef DBG_KERNELMODE

#ifdef _WIN32
              #include <windows.h>
#endif

              /*! This macro is called to output the buffer */
              #define _DBG_PERFORM(_xxx) OutputDebugString(_xxx);

              /*! What has to be defined at the start of each function? */
              #define FUNC_DEF

              /*! Some additional work when the debug buffer is initialized.
                \n In user-mode, nothing has to be done. */
              #define _DBG_START_ADD

              /*! Some additional work when we are done with the debug buffer.
                \n In user-mode, nothing has to be done. */
              #define _DBG_END_ADD

              /*! Get the number of the debug buffer.
              \n In user-mode, it is always 0. */
              #define DEBUG_BUFFER_NO    0

#endif // #ifdef DBG_KERNELMODE

       /*! The prefix which has to be used to any call of DbgOutputIntoBuffer(),
              _DBGO(), and the various other DBG_xxx functions. */
       #define DBG_PREFIX         DEBUG_BUFFER_NO,

       /*! Write something into the debug buffer */
       #define _DBGO(_xxx) DbgOutputIntoBuffer _xxx

#ifdef DBG_KERNELMODE

       /*! start the debugging output line */
       #define _DBG_START() _DBG_START_ADD DbgBufferPos[DEBUG_BUFFER_NO] = 0; DbgOutputIntoBuffer(DBG_PREFIX "%s(%u,%02x)," __FUNCTION__ "(%u): ", DBG_PROGNAME, CbmGetCurrentProcessorNumber(), DebugBufferNo, __LINE__)

#else // #if DBG_KERNELMODE

       /*! start the debugging output line */
       #define _DBG_START() _DBG_START_ADD DbgBufferPos[DEBUG_BUFFER_NO] = 0; DbgOutputIntoBuffer(DBG_PREFIX "%s," __FUNCTION__ "(%u): ", DBG_PROGNAME, __LINE__)

#endif // #if DBG_KERNELMODE

       /*! end the debugging output line */
       #define _DBG_END()   DbgOutputIntoBuffer(DBG_PREFIX "\n"); _DBG_PERFORM(&DbgBuffer[DEBUG_BUFFER_NO][0]) _DBG_END_ADD

       /*! Output exactly one line to the debugging output */
       #define DBGO(_xxx) { _DBG_START(); _DBGO(_xxx); _DBG_END() }

       /*! set a hard-coded breakpoint */
       #define DBG_BREAKPOINT() { if (ISDBG_BREAK()) { DBG_I_BREAKPOINT(); }; }

       /*! enter a function */
       #define FUNC_ENTER()             FUNC_DEF { if (ISDBG_ENTER()) { DBGO(( DBG_PREFIX "Entering %s", __FUNCTION__ )); } }
       /*! leave the function */
       #define FUNC_LEAVE()             { if (ISDBG_LEAVE()) { DBGO(( DBG_PREFIX "Leaving  %s", __FUNCTION__ )); } return; }
       /*! output the parameters of the function */
       #define FUNC_PARAM( _xxx )       { if (ISDBG_PARAM()) { _DBG_START(); _DBGO(( DBG_PREFIX "Parameter for %s: ", __FUNCTION__ )); _DBGO( _xxx ); _DBG_END() } }

       /*! leave the function with a return value of type BOOL */
       #define FUNC_LEAVE_BOOL(   _xxx ) { const BOOL    _OUT_ = _xxx; if (ISDBG_LEAVE()) { DBGO(( DBG_PREFIX "Leaving  %s with BOOL=%s",   __FUNCTION__, (_OUT_)?"TRUE":"FALSE" )); } return _OUT_; }
       /*! leave the function with a return value of type BOOL */
       #define FUNC_LEAVE_BOOLEAN(_xxx ) { const BOOLEAN _OUT_ = _xxx; if (ISDBG_LEAVE()) { DBGO(( DBG_PREFIX "Leaving  %s with BOOL=%s",   __FUNCTION__, (_OUT_)?"TRUE":"FALSE" )); } return _OUT_; }
       /*! leave the function with a return value of type INT */
       #define FUNC_LEAVE_INT(    _xxx ) { const int     _OUT_ = _xxx; if (ISDBG_LEAVE()) { DBGO(( DBG_PREFIX "Leaving  %s with INT=%u (%d)",    __FUNCTION__, (unsigned int)(_OUT_), (signed int)(_OUT_) )); } return _OUT_; }
       /*! leave the function with a return value of type INT */
       #define FUNC_LEAVE_UINT(   _xxx ) { const unsigned int _OUT_ = _xxx; if (ISDBG_LEAVE()) { DBGO(( DBG_PREFIX "Leaving  %s with INT=%u (%d)",    __FUNCTION__, _OUT_, _OUT_ )); } return _OUT_; }
       /*! leave the function with a return value of type INT */
       #define FUNC_LEAVE_USHORT( _xxx ) { const USHORT  _OUT_ = _xxx; if (ISDBG_LEAVE()) { DBGO(( DBG_PREFIX "Leaving  %s with INT=%u (%d)",    __FUNCTION__, (unsigned int)(_OUT_), (signed int)(_OUT_) )); } return _OUT_; }
       /*! leave the function with a return value of type UCHAR */
       #define FUNC_LEAVE_UCHAR(  _xxx ) { const UCHAR   _OUT_ = _xxx; if (ISDBG_LEAVE()) { DBGO(( DBG_PREFIX "Leaving  %s with UCHAR=%u",  __FUNCTION__, (unsigned int)(_OUT_) )); }  return _OUT_; }
       /*! leave the function with a return value of type HANDLE */
       #define FUNC_LEAVE_HANDLE( _xxx ) { const HANDLE  _OUT_ = _xxx; if (ISDBG_LEAVE()) { DBGO(( DBG_PREFIX "Leaving  %s with HANDLE=%u", __FUNCTION__, (unsigned int)(_OUT_) )); }  return _OUT_; }
       /*! leave the function with a return value of type STRING */
       #define FUNC_LEAVE_STRING( _xxx ) { const char *  _OUT_ = _xxx; if (ISDBG_LEAVE()) { DBGO(( DBG_PREFIX "Leaving  %s with '%s'",      __FUNCTION__,           (_OUT_) )); }      return _xxx;  }
       /*! leave the function with a return value of type ULONG */
       #define FUNC_LEAVE_ULONG(  _xxx ) { const ULONG   _OUT_ = _xxx; if (ISDBG_LEAVE()) { DBGO(( DBG_PREFIX "Leaving  %s with ULONG=%ul", __FUNCTION__, (ULONG)   (_OUT_) )); }      return _OUT_; }
       /*! leave the function with a return value of type ULONG */
       #define FUNC_LEAVE_LONG(  _xxx )  { const LONG    _OUT_ = _xxx; if (ISDBG_LEAVE()) { DBGO(( DBG_PREFIX "Leaving  %s with LONG=%l",   __FUNCTION__, (LONG)    (_OUT_) )); }      return _OUT_; }
       /*! leave the function with a return value of a specific type */
       #define FUNC_LEAVE_TYPE(  _xxx, _TYPE, _FORMAT) { _TYPE _OUT_ = _xxx; if (ISDBG_LEAVE()) { DBGO(( DBG_PREFIX "Leaving  %s with " #_TYPE "=" _FORMAT, __FUNCTION__, (_OUT_) )); }      return _OUT_; }
       /*! leave the function with a return value of a pointer type */
       #define FUNC_LEAVE_PTR(  _xxx, _TYPE )  FUNC_LEAVE_TYPE( _xxx, _TYPE, "0x%p")

       #ifdef DBG_KERNELMODE
              /*! leave the function with a return value of type NTSTATUS (no look-up needed) */
              #define FUNC_LEAVE_NTSTATUS_CONST( _xxx ) { if (ISDBG_LEAVE() || (ISDBG_LEAVE_FAILURE() && (_xxx != STATUS_SUCCESS))) { DBGO(( DBG_PREFIX "Leaving  %s with NTSTATUS=%s", __FUNCTION__, #_xxx )); } return _xxx; }
              /*! leave the function with a return value of type NTSTATUS (look-up needed) */
              #define FUNC_LEAVE_NTSTATUS(       _xxx ) { NTSTATUS _OUT_ = _xxx; if (ISDBG_LEAVE() || (ISDBG_LEAVE_FAILURE() && (_xxx != STATUS_SUCCESS))) { DBGO(( DBG_PREFIX "Leaving  %s with NTSTATUS=%s", __FUNCTION__, DebugNtStatus(_OUT_) )); } return _OUT_; }
              /*! Output if DBGF_ASSERTIRQL is defined */
              #define DBG_IRQL(  _xxx ) { if (!(KeGetCurrentIrql() _xxx)) if (ISDBG_ASSERTIRQL()) { DBGO(( DBG_PREFIX "***IRQL ASSERTION FAILED!***: '%s' in %s:%s(%u)", #_xxx, __FILE__, __FUNCTION__, __LINE__ )); DBG_BREAKPOINT(); } }
       #else
              /*! leave the function with a return value of a HMODULE */
              #define FUNC_LEAVE_HMODULE(  _xxx )  FUNC_LEAVE_TYPE( _xxx, HMODULE, "0x%08x")
       #endif

       /*! Output if DBGF_PPORT is defined */
       #define DBG_PPORT(   _xxx ) { if (ISDBG_PPORT()){  DBGO(  _xxx  ); } }
       /*! Output if DBGF_SUCCESS is defined */
       #define DBG_SUCCESS( _xxx ) { if (ISDBG_SUCCESS()) { _DBG_START(); _DBGO( _xxx ); _DBGO((DBG_PREFIX " SUCCESS")); _DBG_END(); } }
       /*! Output if DBGF_WARN is defined */
       #define DBG_WARN(    _xxx ) { if (ISDBG_WARN())    { _DBG_START(); _DBGO(( DBG_PREFIX "Warning: ")); _DBGO( _xxx ); _DBG_END(); } }
       /*! Output if DBGF_ERROR is defined */
       #define DBG_ERROR(   _xxx ) { if (ISDBG_ERROR())   { _DBG_START(); _DBGO(( DBG_PREFIX "***ERROR***: ")); _DBGO( _xxx ); _DBG_END(); } }
       /*! Output if DBGF_PANIC is defined */
       #define DBG_PANIC(   _xxx ) { if (ISDBG_PANIC())   { _DBG_START(); _DBGO(( DBG_PREFIX "***PANIC***: ")); _DBGO( _xxx ); _DBG_END(); } }
       /*! Output if DBGF_ASSERT is defined */
       #define DBG_ASSERT(  _xxx ) { if (!(_xxx)) if (ISDBG_ASSERT()) { DBGO(( DBG_PREFIX "***ASSERTION FAILED!***: %s in %s:%s(%u)", #_xxx, __FILE__, __FUNCTION__, __LINE__ )); DBG_BREAKPOINT(); } DBG_I_ASSUME(_xxx); }
       /*! Output always */
       #define DBG_PRINT(   _xxx ) { DBGO( _xxx ); }

       /*! Similar to DBG_ASSERT(). However, the argument is executed even on release builds. */
       #define DBG_VERIFY(  _xxx ) DBG_ASSERT( _xxx )

       /*! only execute the command when debugging is compiled in */
       #define DBGDO( _xxx ) _xxx

       #ifdef DBG_KERNELMODE

              //! Initialize the debugging system
              #define DBG_INIT() DbgInit()


              /*! Output if DBGF_IEC is defined */
              #define DBG_IEC(     _xxx ) { if (ISDBG_IEC())  {  DBGO( _xxx ); } }
              /*! Output if DBGF_IRQ is defined */
              #define DBG_IRQ(     _xxx ) { if (ISDBG_IRQ())  {  DBGO(( DBG_PREFIX _xxx )); } }
              /*! Output if DBGF_PORT is defined */
              #define DBG_PORT(    _xxx ) { if (ISDBG_PORT()) {  _DBG_START(); _DBGO((DBG_PREFIX "Port Command: ")); _DBGO(_xxx); _DBG_END(); } }
              /*! Output if DBGF_THREAD is defined */
              #define DBG_THREAD(  _xxx ) /* { if (ISDBG_THREAD())   DBGO(( _xxx )); } */
              /*! Output if DBGF_IRPPATCH is defined */
              #define DBG_IRPPATH( _xxx ) { if (ISDBG_IRPPATH()) { DBGO(_xxx); } }
              /*! Output if DBGF_IRP is defined */
              #define DBG_IRP(     _xxx ) { if (ISDBG_IRP()) { DBGO(( DBG_PREFIX "Got IRP: " #_xxx )); } }
              /*! Output if DBGF_DPC is defined */
              #define DBG_DPC(     _xxx ) { if (ISDBG_DPC()) { DBGO( _xxx ); } }
              /*! Output if DBGF_CABLE is defined */
              #define DBG_CABLE(   _xxx ) { if (ISDBG_CABLE()) { DBGO( _xxx ); } }
       #endif

       extern void cbm_get_debugging_flags(const char * ModuleName);


#else  // #if DBG

       //! On release builds, a dummy
       #define DBG_MEMDUMP(_Where, _Buffer, _Count)

       //! On release builds, a dummy
       #define DBG_BREAKPOINT

       //! On release builds, a dummy
       #define FUNC_ENTER()

       //! On release builds, a dummy
       #define FUNC_LEAVE(       ) return

       //! On release builds, a dummy
       #define FUNC_PARAM(  _xxx )

       //! On release builds, a dummy
       #define FUNC_LEAVE_BOOL(   _xxx ) return _xxx

       //! On release builds, a dummy
       #define FUNC_LEAVE_BOOLEAN(   _xxx ) return _xxx

       //! On release builds, a dummy
       #define FUNC_LEAVE_INT(    _xxx ) return _xxx

       //! On release builds, a dummy
       #define FUNC_LEAVE_UINT(   _xxx ) return _xxx

       //! On release builds, a dummy
       #define FUNC_LEAVE_USHORT(  _xxx ) return _xxx

       //! On release builds, a dummy
       #define FUNC_LEAVE_UCHAR(  _xxx ) return _xxx

       //! On release builds, a dummy
       #define FUNC_LEAVE_HANDLE( _xxx ) return _xxx

       //! On release builds, a dummy
       #define FUNC_LEAVE_STRING( _xxx ) return _xxx

       //! On release builds, a dummy
       #define FUNC_LEAVE_ULONG(  _xxx ) return _xxx

       //! On release builds, a dummy
       #define FUNC_LEAVE_LONG(   _xxx ) return _xxx

       //! On release builds, a dummy
       #define FUNC_LEAVE_TYPE(  _xxx, _TYPE, _FORMAT) return _xxx

       //! On release builds, a dummy
       #define FUNC_LEAVE_PTR(    _xxx, _yyy ) return _xxx

       #ifdef DBG_KERNELMODE
              //! On release builds, a dummy
              #define FUNC_LEAVE_NTSTATUS_CONST( _xxx )  return _xxx

              //! On release builds, a dummy
              #define FUNC_LEAVE_NTSTATUS(       _xxx )  return _xxx
       #endif

       //! On release builds, a dummy
       #define DBG_PREFIX

       //! On release builds, a dummy
       #define DBG_PPORT(   _xxx )

       //! On release builds, a dummy
       #define DBG_SUCCESS( _xxx )

       //! On release builds, a dummy
       #define DBG_WARN(    _xxx )

       //! On release builds, a dummy
       #define DBG_ERROR(   _xxx )

       //! On release builds, a dummy
       #define DBG_PANIC(   _xxx )

       //! On release builds, a dummy
       #define DBG_ASSERT(  _xxx )

       //! On release builds, a dummy
       #define DBG_PRINT(   _xxx )

       //! On release builds, a dummy
       #define DBGDO(       _xxx )

       //! On release builds, a dummy
       #define DBG_VERIFY(  _xxx ) _xxx

       #ifdef DBG_KERNELMODE

              //! We do not need to initialize the debugging system
              #define DBG_INIT()

              //! On release builds, a dummy
              #define DBG_IEC(     _xxx )

              //! On release builds, a dummy
              #define DBG_IRQ(     _xxx )

              //! On release builds, a dummy
              #define DBG_IRQL(    _xxx )

              //! On release builds, a dummy
              #define DBG_PORT(    _xxx )

              //! On release builds, a dummy
              #define DBG_THREAD(  _xxx )

              //! On release builds, a dummy
              #define DBG_IRPPATH( _xxx )

              //! On release builds, a dummy
              #define DBG_IRP(     _xxx )

              //! On release builds, a dummy
              #define DBG_DPC(     _xxx )

              //! On release builds, a dummy
              #define DBG_CABLE(   _xxx )

       #else

              //! On release builds, a dummy
              #define FUNC_LEAVE_HMODULE(  _xxx )  return _xxx;

       #endif

#endif // #if DBG

//! Debug IRPPATH: Processing of the IRP
#define DBG_IRPPATH_PROCESS( _Where_ )  DBG_IRPPATH((DBG_PREFIX "IrpPath: + Processing IRP %08x in " _Where_, (char*)Irp))

//! Debug IRPPATH: Completing of the IRP
#define DBG_IRPPATH_COMPLETE( _Where_ ) DBG_IRPPATH((DBG_PREFIX "IrpPath: - Completing IRP %08x in " _Where_ " with ntStatus = %s", (char*)Irp, (char*)DebugNtStatus(ntStatus)))

//! Debug IRPPATH: Cancelling of the IRP
#define DBG_IRPPATH_CANCEL( _Where_ )   DBG_IRPPATH((DBG_PREFIX "IrpPath: - CANCELLING IRP %08x in " _Where_, (char*)Irp))

//! Debug IRPPATH: Executing of the IRP
#define DBG_IRPPATH_EXECUTE( _Where_ )  DBG_IRPPATH((DBG_PREFIX "IrpPath: = Executing IRP %08x in " _Where_, (char*)Irp))

#endif // #ifndef DBG_H
