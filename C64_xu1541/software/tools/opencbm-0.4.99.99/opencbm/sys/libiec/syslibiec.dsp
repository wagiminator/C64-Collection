# Microsoft Developer Studio Project File - Name="syslibiec" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=syslibiec - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "syslibiec.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "syslibiec.mak" CFG="syslibiec - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "syslibiec - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "syslibiec - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "syslibiec - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f syslibiec.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "syslibiec.exe"
# PROP BASE Bsc_Name "syslibiec.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\..\ddkbuild_start fre"
# PROP Rebuild_Opt "-cZ"
# PROP Target_File "syslibiec.exe"
# PROP Bsc_Name "syslibiec.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "syslibiec - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f syslibiec.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "syslibiec.exe"
# PROP BASE Bsc_Name "syslibiec.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "..\..\ddkbuild_start chk"
# PROP Rebuild_Opt "-cZ"
# PROP Target_File "syslibiec.exe"
# PROP Bsc_Name "syslibiec.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "syslibiec - Win32 Release"
# Name "syslibiec - Win32 Debug"

!IF  "$(CFG)" == "syslibiec - Win32 Release"

!ELSEIF  "$(CFG)" == "syslibiec - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\checkcable.c
# End Source File
# Begin Source File

SOURCE=.\checkdevice.c
# End Source File
# Begin Source File

SOURCE=.\dbgread.c
# End Source File
# Begin Source File

SOURCE=.\dbgwrite.c
# End Source File
# Begin Source File

SOURCE=.\debug.c
# End Source File
# Begin Source File

SOURCE=.\dpc.c
# End Source File
# Begin Source File

SOURCE=.\eoi.c
# End Source File
# Begin Source File

SOURCE=.\i_rawread.c
# End Source File
# Begin Source File

SOURCE=.\i_rawwrite.c
# End Source File
# Begin Source File

SOURCE=.\init.c
# End Source File
# Begin Source File

SOURCE=.\interrupt.c
# End Source File
# Begin Source File

SOURCE=.\listen.c
# End Source File
# Begin Source File

SOURCE=.\mnib.c
# End Source File
# Begin Source File

SOURCE=.\openclose.c
# End Source File
# Begin Source File

SOURCE=.\poll.c
# End Source File
# Begin Source File

SOURCE=.\ppread.c
# End Source File
# Begin Source File

SOURCE=.\ppwrite.c
# End Source File
# Begin Source File

SOURCE=.\rawread.c
# End Source File
# Begin Source File

SOURCE=.\rawwrite.c
# End Source File
# Begin Source File

SOURCE=.\release.c
# End Source File
# Begin Source File

SOURCE=.\releasebus.c
# End Source File
# Begin Source File

SOURCE=.\reset.c
# End Source File
# Begin Source File

SOURCE=.\sendbyte.c
# End Source File
# Begin Source File

SOURCE=.\set.c
# End Source File
# Begin Source File

SOURCE=.\setrelease.c
# End Source File
# Begin Source File

SOURCE=.\talk.c
# End Source File
# Begin Source File

SOURCE=.\testirq.c
# End Source File
# Begin Source File

SOURCE=.\unlisten.c
# End Source File
# Begin Source File

SOURCE=.\untalk.c
# End Source File
# Begin Source File

SOURCE=.\util.c
# End Source File
# Begin Source File

SOURCE=.\wait.c
# End Source File
# Begin Source File

SOURCE=.\waitlistener.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\include\WINDOWS\arch_cbm_driver.h
# End Source File
# Begin Source File

SOURCE=..\include\cbm_driver.h
# End Source File
# Begin Source File

SOURCE=..\..\include\debug.h
# End Source File
# Begin Source File

SOURCE=.\i_iec.h
# End Source File
# Begin Source File

SOURCE=..\include\iec.h
# End Source File
# Begin Source File

SOURCE=..\include\memtags.h
# End Source File
# Begin Source File

SOURCE=..\include\util.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\.cvsignore
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# End Target
# End Project
