# Microsoft Developer Studio Project File - Name="opencbmxa1541" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=opencbmxa1541 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "opencbmxa1541.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "opencbmxa1541.mak" CFG="opencbmxa1541 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "opencbmxa1541 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "opencbmxa1541 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "opencbmxa1541 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../../../../Release"
# PROP Intermediate_Dir "../../../../Release/opencbm-xa1541"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DLL_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "../" /I "../../../../include" /I "../../../../include/WINDOWS/" /I "../../../../lib/WINDOWS/" /I "../../../" /I "../../../../arch/windows/" /I "$(WINDDK)\inc\wxp" /I "$(WINDDK)\inc\crt" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DLL_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /i "../../../../include" /i "../../../../include/WINDOWS/" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib arch.lib winmm.lib setupapi.lib version.lib /nologo /dll /machine:I386 /nodefaultlib:"libc.lib" /libpath:"../../../../Release"
# Begin Special Build Tool
OutDir=.\../../../../Release
SOURCE="$(InputPath)"
PreLink_Cmds=del "$(OutDir)\opencbm-xa1541.dll"
PostBuild_Cmds=copy "$(OutDir)\opencbmxa1541.dll" "$(OutDir)\opencbm-xa1541.dll"
# End Special Build Tool

!ELSEIF  "$(CFG)" == "opencbmxa1541 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../../../../Debug"
# PROP Intermediate_Dir "../../../../Debug/opencbm-xa1541"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DLL_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../" /I "../../../../include" /I "../../../../include/WINDOWS/" /I "../../../../lib/WINDOWS/" /I "../../../" /I "../../../../arch/windows/" /I "$(WINDDK)\inc\wxp" /I "$(WINDDK)\inc\crt" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DLL_EXPORTS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /i "../../../../include" /i "../../../../include/WINDOWS/" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib arch.lib winmm.lib setupapi.lib version.lib /nologo /dll /debug /machine:I386 /nodefaultlib:"libcd.lib" /pdbtype:sept /libpath:"../../../../Debug"
# Begin Special Build Tool
OutDir=.\../../../../Debug
SOURCE="$(InputPath)"
PreLink_Cmds=del "$(OutDir)\opencbm-xa1541.dll"
PostBuild_Cmds=copy "$(OutDir)\opencbmxa1541.dll" "$(OutDir)\opencbm-xa1541.dll"
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "opencbmxa1541 - Win32 Release"
# Name "opencbmxa1541 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\i_opencbm.c
# End Source File
# Begin Source File

SOURCE=.\iec.c
# End Source File
# Begin Source File

SOURCE=.\install.c
# End Source File
# Begin Source File

SOURCE=.\parburst.c
# End Source File
# Begin Source File

SOURCE=.\parport.c
# End Source File
# Begin Source File

SOURCE=.\service.c
# End Source File
# Begin Source File

SOURCE=.\startstop.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\archlib.h
# End Source File
# Begin Source File

SOURCE=.\i_opencbm.h
# End Source File
# Begin Source File

SOURCE="..\..\..\..\include\opencbm-plugin.h"
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=".\opencbm-xa1541.rc"
# End Source File
# End Group
# Begin Source File

SOURCE=.\makefile
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# Begin Source File

SOURCE=..\..\..\..\include\WINDOWS\version.common.h
# End Source File
# End Target
# End Project
