# Microsoft Developer Studio Project File - Name="trestatic" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=trestatic - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "trestatic.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "trestatic.mak" CFG="trestatic - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "trestatic - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "trestatic - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "trestatic - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "trestatic___Win32_Release"
# PROP BASE Intermediate_Dir "trestatic___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../win32" /I "../lib" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_LIB" /D "TRE_EXPORTS" /D "HAVE_CONFIG_H" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "trestatic - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "trestatic___Win32_Debug"
# PROP BASE Intermediate_Dir "trestatic___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../win32" /I "../lib" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_LIB" /D "TRE_EXPORTS" /D "TRE_DEBUG" /D "HAVE_CONFIG_H" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "trestatic - Win32 Release"
# Name "trestatic - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\lib\regcomp.c
# End Source File
# Begin Source File

SOURCE=..\lib\regerror.c
# End Source File
# Begin Source File

SOURCE=..\lib\regexec.c
# End Source File
# Begin Source File

SOURCE="..\lib\tre-ast.c"
# End Source File
# Begin Source File

SOURCE="..\lib\tre-compile.c"
# End Source File
# Begin Source File

SOURCE="..\lib\tre-filter.c"
# End Source File
# Begin Source File

SOURCE="..\lib\tre-match-approx.c"
# End Source File
# Begin Source File

SOURCE="..\lib\tre-match-backtrack.c"
# End Source File
# Begin Source File

SOURCE="..\lib\tre-match-parallel.c"
# End Source File
# Begin Source File

SOURCE="..\lib\tre-mem.c"
# End Source File
# Begin Source File

SOURCE="..\lib\tre-parse.c"
# End Source File
# Begin Source File

SOURCE="..\lib\tre-stack.c"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\config.h
# End Source File
# Begin Source File

SOURCE=..\lib\gettext.h
# End Source File
# Begin Source File

SOURCE=..\lib\regex.h
# End Source File
# Begin Source File

SOURCE=".\tre-config.h"
# End Source File
# Begin Source File

SOURCE="..\lib\tre-internal.h"
# End Source File
# Begin Source File

SOURCE="..\lib\tre-match-utils.h"
# End Source File
# Begin Source File

SOURCE="..\lib\tre-mem.h"
# End Source File
# Begin Source File

SOURCE=..\lib\xmalloc.h
# End Source File
# End Group
# End Target
# End Project
