# Microsoft Developer Studio Project File - Name="opencbmxum1541" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=opencbmxum1541 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "opencbmxum1541.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "opencbmxum1541.mak" CFG="opencbmxum1541 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "opencbmxum1541 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "opencbmxum1541 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "opencbmxum1541 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../../../../Release"
# PROP Intermediate_Dir "../../../../Release/opencbm-xum1541"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DLL_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "../../../../lib/WINDOWS/" /I "../../../../libmisc/WINDOWS/" /I "../" /I "../../../" /I "../../../../include" /I "../../../../include/WINDOWS/" /I "../../../../libmisc/" /I "../../../../../xum1541/" /I "../../../../arch/windows/" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DLL_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /i "../../../../include" /i "../../../../include/WINDOWS/" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib arch.lib winmm.lib /nologo /dll /machine:I386 /nodefaultlib:"libc.lib" /libpath:"../../../../Release"
# Begin Special Build Tool
OutDir=.\../../../../Release
SOURCE="$(InputPath)"
PreLink_Cmds=del "$(OutDir)\opencbm-xum1541.dll"
PostBuild_Cmds=copy "$(OutDir)\opencbmxum1541.dll" "$(OutDir)\opencbm-xum1541.dll"
# End Special Build Tool

!ELSEIF  "$(CFG)" == "opencbmxum1541 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../../../../Debug"
# PROP Intermediate_Dir "../../../../Debug/opencbm-xum1541"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DLL_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../../../lib/WINDOWS/" /I "../../../../libmisc/WINDOWS/" /I "../" /I "../../../" /I "../../../../include" /I "../../../../include/WINDOWS/" /I "../../../../../xum1541/" /I "../../../../arch/windows/" /I "../../../../libmisc/" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DLL_EXPORTS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /i "../../../../include" /i "../../../../include/WINDOWS/" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib arch.lib winmm.lib /nologo /dll /debug /machine:I386 /nodefaultlib:"libcd.lib" /pdbtype:sept /libpath:"../../../../Debug"
# Begin Special Build Tool
OutDir=.\../../../../Debug
SOURCE="$(InputPath)"
PreLink_Cmds=del "$(OutDir)\opencbm-xum1541.dll"
PostBuild_Cmds=copy "$(OutDir)\opencbmxum1541.dll" "$(OutDir)\opencbm-xum1541.dll"
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "opencbmxum1541 - Win32 Release"
# Name "opencbmxum1541 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\archlib.c
# End Source File
# Begin Source File

SOURCE=.\dllmain.c
# End Source File
# Begin Source File

SOURCE=.\install.c
# End Source File
# Begin Source File

SOURCE=..\s1_s2_pp.c
# End Source File
# Begin Source File

SOURCE=..\xum1541.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\archlib.h
# End Source File
# Begin Source File

SOURCE="..\..\..\..\include\opencbm-plugin.h"
# End Source File
# Begin Source File

SOURCE=.\usb.h
# End Source File
# Begin Source File

SOURCE=..\xum1541.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=".\opencbm-xum1541.rc"
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
