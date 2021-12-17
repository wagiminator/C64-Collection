# Microsoft Developer Studio Project File - Name="sysnt4" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=sysnt4 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "sysnt4.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sysnt4.mak" CFG="sysnt4 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sysnt4 - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "sysnt4 - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "sysnt4 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f sysnt4.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "sysnt4.exe"
# PROP BASE Bsc_Name "sysnt4.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\..\ddkbuild_start fre"
# PROP Rebuild_Opt "-cZ"
# PROP Target_File "sysnt4"
# PROP Bsc_Name "sysnt4driver.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "sysnt4 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f sysnt4.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "sysnt4.exe"
# PROP BASE Bsc_Name "sysnt4.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "..\..\ddkbuild_start chk"
# PROP Rebuild_Opt "-cZ"
# PROP Target_File "sysnt4"
# PROP Bsc_Name "sysnt4driver.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "sysnt4 - Win32 Release"
# Name "sysnt4 - Win32 Debug"

!IF  "$(CFG)" == "sysnt4 - Win32 Release"

!ELSEIF  "$(CFG)" == "sysnt4 - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\LoadUnload.c
# End Source File
# Begin Source File

SOURCE=.\PortAccessNt4.c
# End Source File
# Begin Source File

SOURCE=.\PortEnum.c
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

SOURCE=..\include\memtags.h
# End Source File
# Begin Source File

SOURCE=..\include\util.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\cbm4nt.rc
# End Source File
# Begin Source File

SOURCE=.\cbmlog.rc
# End Source File
# End Group
# Begin Source File

SOURCE=.\.cvsignore
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# End Target
# End Project
