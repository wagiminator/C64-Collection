# Microsoft Developer Studio Project File - Name="libtrans" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libtrans - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libtrans.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libtrans.mak" CFG="libtrans - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libtrans - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libtrans - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libtrans - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../../Release"
# PROP Intermediate_Dir "../../Release/libtrans"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "../../include" /I "../../include/WINDOWS/" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /i "../../include" /i "../../include/WINDOWS/" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libtrans - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../../Debug"
# PROP Intermediate_Dir "../../Debug/libtrans"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../../include" /I "../../include/WINDOWS/" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /i "../../include" /i "../../include/WINDOWS/" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "libtrans - Win32 Release"
# Name "libtrans - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\o65.c
# End Source File
# Begin Source File

SOURCE=..\pp.c
# End Source File
# Begin Source File

SOURCE=..\s1.c
# End Source File
# Begin Source File

SOURCE=..\s2.c
# End Source File
# Begin Source File

SOURCE=..\turbo.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\include\debug.h
# End Source File
# Begin Source File

SOURCE=..\..\include\libtrans.h
# End Source File
# Begin Source File

SOURCE=..\libtrans_int.h
# End Source File
# Begin Source File

SOURCE=..\o65.h
# End Source File
# Begin Source File

SOURCE=..\o65_int.h
# End Source File
# Begin Source File

SOURCE=..\..\include\opencbm.h
# End Source File
# End Group
# Begin Group "CA65"

# PROP Default_Filter "a65"
# Begin Source File

SOURCE=..\common.i65
# End Source File
# Begin Source File

SOURCE=..\pp1541.a65

!IF  "$(CFG)" == "libtrans - Win32 Release"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libtrans
InputPath=..\pp1541.a65
InputName=pp1541

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libtrans - Win32 Debug"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libtrans
InputPath=..\pp1541.a65
InputName=pp1541

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\pp1571.a65

!IF  "$(CFG)" == "libtrans - Win32 Release"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libtrans
InputPath=..\pp1571.a65
InputName=pp1571

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libtrans - Win32 Debug"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libtrans
InputPath=..\pp1571.a65
InputName=pp1571

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\s1.a65

!IF  "$(CFG)" == "libtrans - Win32 Release"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libtrans
InputPath=..\s1.a65
InputName=s1

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libtrans - Win32 Debug"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libtrans
InputPath=..\s1.a65
InputName=s1

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\s2.a65

!IF  "$(CFG)" == "libtrans - Win32 Release"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libtrans
InputPath=..\s2.a65
InputName=s2

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libtrans - Win32 Debug"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libtrans
InputPath=..\s2.a65
InputName=s2

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\turbomain.a65

!IF  "$(CFG)" == "libtrans - Win32 Release"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libtrans
InputPath=..\turbomain.a65
InputName=turbomain

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libtrans - Win32 Debug"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libtrans
InputPath=..\turbomain.a65
InputName=turbomain

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\makefile
# End Source File
# Begin Source File

SOURCE=.\Makefile.inc
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# End Target
# End Project
