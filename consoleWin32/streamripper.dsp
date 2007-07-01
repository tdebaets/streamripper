# Microsoft Developer Studio Project File - Name="streamripper" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=streamripper - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "streamripper.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "streamripper.mak" CFG="streamripper - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "streamripper - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "streamripper - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "streamripper - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\libogg-1.1.3" /I "..\libvorbis-1.1.2" /I "..\libmad-0.15.1b\msvc++" /I "..\lib" /I "..\iconv-win32\static" /I "..\tre-0.7.2\lib" /I "..\tre-0.7.2\win32" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 MSVCRT.LIB kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib Ws2_32.lib libmad.lib charset.lib iconv.lib tre.lib ogg.lib vorbis.lib /nologo /subsystem:console /machine:I386 /out:"Release/streamripper.exe" /libpath:"..\libogg-1.1.3" /libpath:"..\libvorbis-1.1.2" /libpath:"..\libmad-0.15.1b\msvc++\Release" /libpath:"..\iconv-win32\static" /libpath:"..\tre-0.7.2\win32\Release"
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "streamripper - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /I "..\libogg-1.1.3" /I "..\libvorbis-1.1.2" /I "..\libmad-0.15.1b\msvc++" /I "..\lib" /I "..\iconv-win32-1.11" /I "..\tre-0.7.2\lib" /I "..\tre-0.7.2\win32" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib Ws2_32.lib libmad.lib tre.lib ogg.lib vorbis.lib charset.lib iconv.lib /nologo /subsystem:console /debug /machine:I386 /out:"Debug/streamripper.exe" /pdbtype:sept /libpath:"..\libogg-1.1.3" /libpath:"..\libvorbis-1.1.2" /libpath:"..\libmad-0.15.1b\msvc++\Debug" /libpath:"..\iconv-win32-1.11" /libpath:"..\tre-0.7.2\win32\Release"

!ENDIF 

# Begin Target

# Name "streamripper - Win32 Release"
# Name "streamripper - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\console\streamripper.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "libs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\lib\cbuf2.c
# End Source File
# Begin Source File

SOURCE=..\lib\cbuf2.h
# End Source File
# Begin Source File

SOURCE=..\lib\compat.h
# End Source File
# Begin Source File

SOURCE=..\lib\confw32.h
# End Source File
# Begin Source File

SOURCE=..\lib\debug.c
# End Source File
# Begin Source File

SOURCE=..\lib\debug.h
# End Source File
# Begin Source File

SOURCE=..\lib\external.c
# End Source File
# Begin Source File

SOURCE=..\lib\external.h
# End Source File
# Begin Source File

SOURCE=..\lib\filelib.c
# End Source File
# Begin Source File

SOURCE=..\lib\filelib.h
# End Source File
# Begin Source File

SOURCE=..\lib\findsep.c
# End Source File
# Begin Source File

SOURCE=..\lib\findsep.h
# End Source File
# Begin Source File

SOURCE=..\lib\http.c
# End Source File
# Begin Source File

SOURCE=..\lib\http.h
# End Source File
# Begin Source File

SOURCE=..\lib\list.h
# End Source File
# Begin Source File

SOURCE=..\lib\mchar.c
# End Source File
# Begin Source File

SOURCE=..\lib\mchar.h
# End Source File
# Begin Source File

SOURCE=..\lib\mpeg.c
# End Source File
# Begin Source File

SOURCE=..\lib\mpeg.h
# End Source File
# Begin Source File

SOURCE=..\lib\parse.c
# End Source File
# Begin Source File

SOURCE=..\lib\parse.h
# End Source File
# Begin Source File

SOURCE=..\lib\relaylib.c
# End Source File
# Begin Source File

SOURCE=..\lib\relaylib.h
# End Source File
# Begin Source File

SOURCE=..\lib\rip_manager.c
# End Source File
# Begin Source File

SOURCE=..\lib\rip_manager.h
# End Source File
# Begin Source File

SOURCE=..\lib\ripaac.c
# End Source File
# Begin Source File

SOURCE=..\lib\ripogg.c
# End Source File
# Begin Source File

SOURCE=..\lib\ripogg.h
# End Source File
# Begin Source File

SOURCE=..\lib\ripstream.c
# End Source File
# Begin Source File

SOURCE=..\lib\ripstream.h
# End Source File
# Begin Source File

SOURCE=..\lib\socklib.c
# End Source File
# Begin Source File

SOURCE=..\lib\socklib.h
# End Source File
# Begin Source File

SOURCE=..\lib\srconfig.h
# End Source File
# Begin Source File

SOURCE=..\lib\srtypes.h
# End Source File
# Begin Source File

SOURCE=..\lib\threadlib.c
# End Source File
# Begin Source File

SOURCE=..\lib\threadlib.h
# End Source File
# Begin Source File

SOURCE=..\lib\types.h
# End Source File
# Begin Source File

SOURCE=..\lib\uce_dirent.h
# End Source File
# Begin Source File

SOURCE=..\lib\utf8.c
# End Source File
# Begin Source File

SOURCE=..\lib\utf8.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\Changes
# End Source File
# Begin Source File

SOURCE=..\configure.ac
# End Source File
# Begin Source File

SOURCE=..\gcs_notes.txt
# End Source File
# Begin Source File

SOURCE=..\makedist_win.bat
# End Source File
# Begin Source File

SOURCE=..\winamp_plugin\sr2x.nsi
# End Source File
# Begin Source File

SOURCE=..\lib\util.h
# End Source File
# End Target
# End Project
