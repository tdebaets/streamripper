# Microsoft Developer Studio Project File - Name="sripper" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=sripper - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "sripper.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sripper.mak" CFG="sripper - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sripper - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "sripper - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "sripper - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\studio\wacs"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "sripper_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\Wasabi SDK\studio" /I "c:\sripper_1x\libmad" /I "..\\" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "sripper_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libmad.lib ws2_32.lib comctl32.lib /nologo /dll /machine:I386 /out:"C:\Program Files\Winamp3\Wacs\sripper.wac" /libpath:"C:\sripper_1x\libmad\Release"

!ELSEIF  "$(CFG)" == "sripper - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\studio\wacs"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "sripper_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\Wasabi SDK\studio" /I "c:\sripper_1x\libmad" /I "..\\" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "sripper_EXPORTS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libmad.lib ws2_32.lib comctl32.lib /nologo /dll /debug /machine:I386 /out:"C:\Program Files\Winamp3\Wacs\sripper.wac" /pdbtype:sept /libpath:"C:\sripper_1x\libmad\Debug"
# SUBTRACT LINK32 /verbose

!ENDIF 

# Begin Target

# Name "sripper - Win32 Release"
# Name "sripper - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\options.c
# End Source File
# Begin Source File

SOURCE=.\sripper.cpp
# End Source File
# Begin Source File

SOURCE=.\sripper.rc
# End Source File
# Begin Source File

SOURCE=.\sripperwnd.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\options.h
# End Source File
# Begin Source File

SOURCE=.\sripper.h
# End Source File
# Begin Source File

SOURCE=.\sripperwnd.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=".\exa-active.png"
# End Source File
# Begin Source File

SOURCE=".\exa-hover.png"
# End Source File
# Begin Source File

SOURCE=".\exa-unactive.png"
# End Source File
# Begin Source File

SOURCE=.\sricon.ico
# End Source File
# End Group
# Begin Group "srlib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\..\sripper_1x\lib\cbuffer.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\sripper_1x\lib\cbuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\sripper_1x\lib\compat.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\sripper_1x\lib\debug.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\sripper_1x\lib\debug.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\sripper_1x\lib\filelib.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\sripper_1x\lib\filelib.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\sripper_1x\lib\findsep.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\sripper_1x\lib\findsep.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\sripper_1x\lib\http.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\sripper_1x\lib\http.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\sripper_1x\lib\inet.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\sripper_1x\lib\inet.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\sripper_1x\lib\mpeg.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\sripper_1x\lib\mpeg.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\sripper_1x\lib\relaylib.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\sripper_1x\lib\relaylib.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\sripper_1x\lib\rip_manager.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\sripper_1x\lib\rip_manager.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\sripper_1x\lib\ripshout.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\sripper_1x\lib\ripshout.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\sripper_1x\lib\ripstream.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\sripper_1x\lib\ripstream.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\sripper_1x\lib\socklib.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\sripper_1x\lib\socklib.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\sripper_1x\lib\threadlib.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\sripper_1x\lib\threadlib.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\sripper_1x\lib\types.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\sripper_1x\lib\util.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\sripper_1x\lib\util.h
# End Source File
# End Group
# Begin Source File

SOURCE=".\sr-active.png"
# End Source File
# Begin Source File

SOURCE=".\sr-hover.png"
# End Source File
# Begin Source File

SOURCE=".\sr-unactive.png"
# End Source File
# End Target
# End Project
