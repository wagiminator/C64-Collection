# Microsoft Developer Studio Project File - Name="syslibcommon" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=syslibcommon - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "syslibcommon.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "syslibcommon.mak" CFG="syslibcommon - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "syslibcommon - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "syslibcommon - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "syslibcommon - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f syslibcommon.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "syslibcommon.exe"
# PROP BASE Bsc_Name "syslibcommon.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\..\ddkbuild_start fre"
# PROP Rebuild_Opt "-cZ"
# PROP Target_File "syslibcommon.lib"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "syslibcommon - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f syslibcommon.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "syslibcommon.exe"
# PROP BASE Bsc_Name "syslibcommon.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "..\..\ddkbuild_start chk"
# PROP Rebuild_Opt "-cZ"
# PROP Target_File "syslibcommon.lib"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "syslibcommon - Win32 Release"
# Name "syslibcommon - Win32 Debug"

!IF  "$(CFG)" == "syslibcommon - Win32 Release"

!ELSEIF  "$(CFG)" == "syslibcommon - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\cleanup.c
# End Source File
# Begin Source File

SOURCE=.\debug.c
# End Source File
# Begin Source File

SOURCE=.\init.c
# End Source File
# Begin Source File

SOURCE=.\install.c
# End Source File
# Begin Source File

SOURCE=.\ioctl.c
# End Source File
# Begin Source File

SOURCE=.\isr.c
# End Source File
# Begin Source File

SOURCE=.\lockunlock.c
# End Source File
# Begin Source File

SOURCE=.\openclose.c
# End Source File
# Begin Source File

SOURCE=.\perfeval.c
# End Source File
# Begin Source File

SOURCE=.\PortAccess.c
# End Source File
# Begin Source File

SOURCE=.\queue.c
# End Source File
# Begin Source File

SOURCE=.\readwrite.c
# End Source File
# Begin Source File

SOURCE=.\startio.c
# End Source File
# Begin Source File

SOURCE=.\thread.c
# End Source File
# Begin Source File

SOURCE=".\util-reg.c"
# End Source File
# Begin Source File

SOURCE=.\util.c
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

SOURCE=..\..\include\WINDOWS\cbmioctl.h
# End Source File
# Begin Source File

SOURCE=..\..\include\debug.h
# End Source File
# Begin Source File

SOURCE=..\include\iec.h
# End Source File
# Begin Source File

SOURCE=..\include\memtags.h
# End Source File
# Begin Source File

SOURCE=..\..\include\WINDOWS\perfeval.h
# End Source File
# Begin Source File

SOURCE=..\include\queue.h
# End Source File
# Begin Source File

SOURCE=..\include\util.h
# End Source File
# Begin Source File

SOURCE=..\..\include\version.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "i386"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\i386\clisti.c
# End Source File
# End Group
# Begin Group "AMD64"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\amd64\clisti.c
# End Source File
# End Group
# Begin Group "iA64"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ia64\clisti.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\.cvsignore
# End Source File
# Begin Source File

SOURCE=.\cbmlog.mc
# End Source File
# Begin Source File

SOURCE=.\makefile.inc
# End Source File
# Begin Source File

SOURCE=..\..\include\opencbm.h
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# End Target
# End Project
