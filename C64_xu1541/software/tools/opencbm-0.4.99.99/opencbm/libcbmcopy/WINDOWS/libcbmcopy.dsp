# Microsoft Developer Studio Project File - Name="libcbmcopy" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libcbmcopy - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libcbmcopy.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libcbmcopy.mak" CFG="libcbmcopy - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libcbmcopy - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libcbmcopy - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libcbmcopy - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../../Release"
# PROP Intermediate_Dir "../../Release/libcbmcopy"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "../../include" /I "../../include/WINDOWS/" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libcbmcopy - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../../Debug"
# PROP Intermediate_Dir "../../Debug/libcbmcopy"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../../include" /I "../../include/WINDOWS/" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "libcbmcopy - Win32 Release"
# Name "libcbmcopy - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\cbmcopy.c
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

SOURCE=..\std.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\include\cbmcopy.h
# End Source File
# Begin Source File

SOURCE=..\cbmcopy_int.h
# End Source File
# End Group
# Begin Group "CA65"

# PROP Default_Filter "a65"
# Begin Source File

SOURCE="..\ppr-1541.a65"

!IF  "$(CFG)" == "libcbmcopy - Win32 Release"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath="..\ppr-1541.a65"
InputName=ppr-1541

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libcbmcopy - Win32 Debug"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath="..\ppr-1541.a65"
InputName=ppr-1541

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\ppr-1571.a65"

!IF  "$(CFG)" == "libcbmcopy - Win32 Release"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath="..\ppr-1571.a65"
InputName=ppr-1571

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libcbmcopy - Win32 Debug"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath="..\ppr-1571.a65"
InputName=ppr-1571

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\ppw-1541.a65"

!IF  "$(CFG)" == "libcbmcopy - Win32 Release"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath="..\ppw-1541.a65"
InputName=ppw-1541

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libcbmcopy - Win32 Debug"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath="..\ppw-1541.a65"
InputName=ppw-1541

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\ppw-1571.a65"

!IF  "$(CFG)" == "libcbmcopy - Win32 Release"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath="..\ppw-1571.a65"
InputName=ppw-1571

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libcbmcopy - Win32 Debug"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath="..\ppw-1571.a65"
InputName=ppw-1571

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\s1r-1581.a65"

!IF  "$(CFG)" == "libcbmcopy - Win32 Release"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath="..\s1r-1581.a65"
InputName=s1r-1581

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libcbmcopy - Win32 Debug"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath="..\s1r-1581.a65"
InputName=s1r-1581

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\s1r.a65

!IF  "$(CFG)" == "libcbmcopy - Win32 Release"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath=..\s1r.a65
InputName=s1r

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libcbmcopy - Win32 Debug"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath=..\s1r.a65
InputName=s1r

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\s1w-1581.a65"

!IF  "$(CFG)" == "libcbmcopy - Win32 Release"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath="..\s1w-1581.a65"
InputName=s1w-1581

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libcbmcopy - Win32 Debug"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath="..\s1w-1581.a65"
InputName=s1w-1581

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\s1w.a65

!IF  "$(CFG)" == "libcbmcopy - Win32 Release"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath=..\s1w.a65
InputName=s1w

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libcbmcopy - Win32 Debug"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath=..\s1w.a65
InputName=s1w

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\s2r-1581.a65"

!IF  "$(CFG)" == "libcbmcopy - Win32 Release"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath="..\s2r-1581.a65"
InputName=s2r-1581

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libcbmcopy - Win32 Debug"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath="..\s2r-1581.a65"
InputName=s2r-1581

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\s2r.a65

!IF  "$(CFG)" == "libcbmcopy - Win32 Release"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath=..\s2r.a65
InputName=s2r

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libcbmcopy - Win32 Debug"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath=..\s2r.a65
InputName=s2r

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\s2w-1581.a65"

!IF  "$(CFG)" == "libcbmcopy - Win32 Release"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath="..\s2w-1581.a65"
InputName=s2w-1581

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libcbmcopy - Win32 Debug"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath="..\s2w-1581.a65"
InputName=s2w-1581

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\s2w.a65

!IF  "$(CFG)" == "libcbmcopy - Win32 Release"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath=..\s2w.a65
InputName=s2w

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libcbmcopy - Win32 Debug"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath=..\s2w.a65
InputName=s2w

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\turboread1541.a65

!IF  "$(CFG)" == "libcbmcopy - Win32 Release"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath=..\turboread1541.a65
InputName=turboread1541

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libcbmcopy - Win32 Debug"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath=..\turboread1541.a65
InputName=turboread1541

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\turboread1571.a65

!IF  "$(CFG)" == "libcbmcopy - Win32 Release"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath=..\turboread1571.a65
InputName=turboread1571

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libcbmcopy - Win32 Debug"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath=..\turboread1571.a65
InputName=turboread1571

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\turboread1581.a65

!IF  "$(CFG)" == "libcbmcopy - Win32 Release"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath=..\turboread1581.a65
InputName=turboread1581

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libcbmcopy - Win32 Debug"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath=..\turboread1581.a65
InputName=turboread1581

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\turbowrite1541.a65

!IF  "$(CFG)" == "libcbmcopy - Win32 Release"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath=..\turbowrite1541.a65
InputName=turbowrite1541

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libcbmcopy - Win32 Debug"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath=..\turbowrite1541.a65
InputName=turbowrite1541

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\turbowrite1571.a65

!IF  "$(CFG)" == "libcbmcopy - Win32 Release"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath=..\turbowrite1571.a65
InputName=turbowrite1571

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libcbmcopy - Win32 Debug"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath=..\turbowrite1571.a65
InputName=turbowrite1571

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\turbowrite1581.a65

!IF  "$(CFG)" == "libcbmcopy - Win32 Release"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath=..\turbowrite1581.a65
InputName=turbowrite1581

"$(InputDir)\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\WINDOWS\buildoneinc ..\.. $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libcbmcopy - Win32 Debug"

# Begin Custom Build
InputDir=\cygwin\home\tri\cbm\opencbm\libcbmcopy
InputPath=..\turbowrite1581.a65
InputName=turbowrite1581

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

SOURCE=.\sources
# End Source File
# End Target
# End Project
