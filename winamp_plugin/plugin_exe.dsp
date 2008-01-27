# Microsoft Developer Studio Project File - Name="plugin_exe" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=plugin_exe - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "plugin_exe.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "plugin_exe.mak" CFG="plugin_exe - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "plugin_exe - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "plugin_exe - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "plugin_exe - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "plugin_exe___Win32_Release"
# PROP BASE Intermediate_Dir "plugin_exe___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../libogg-1.1.3" /I "../libvorbis-1.1.2" /I "../lib" /I "../libmad-0.15.1b/msvc++" /I "..\win32\glib-2.12.12\include\glib-2.0" /I "..\win32\glib-2.12.12\lib\glib-2.0\include" /I "../iconv-win32/static" /I "../tre-0.7.2/lib" /I "../tre-0.7.2/win32" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib comctl32.lib libmad.lib tre.lib ogg.lib vorbis.lib charset.lib iconv.lib glib-2.0.lib /nologo /subsystem:windows /machine:I386 /out:"c:/Program files/streamripper-1.63-beta-2/wstreamripper.exe" /libpath:"../libmad-0.15.1b/msvc++/release" /libpath:"../tre-0.7.2/win32/release" /libpath:"../libogg-1.1.3" /libpath:"../libvorbis-1.1.2" /libpath:"../iconv-win32/static" /libpath:"..\win32\glib-2.12.12\lib"

!ELSEIF  "$(CFG)" == "plugin_exe - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "plugin_exe___Win32_Debug"
# PROP BASE Intermediate_Dir "plugin_exe___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /I "../libogg-1.1.3" /I "../libvorbis-1.1.2" /I "../lib" /I "../libmad-0.15.1b/msvc++" /I "..\win32\glib-2.12.12\include\glib-2.0" /I "..\win32\glib-2.12.12\lib\glib-2.0\include" /I "../iconv-win32/static" /I "../tre-0.7.2/lib" /I "../tre-0.7.2/win32" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Fr /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib comctl32.lib msvcrt.lib libmad.lib tre.lib ogg.lib vorbis.lib charset.lib iconv.lib glib-2.0.lib /nologo /subsystem:windows /debug /machine:I386 /out:"c:/Program files/streamripper/wstreamripper.exe" /pdbtype:sept /libpath:"../libmad-0.15.1b/msvc++/debug" /libpath:"../tre-0.7.2/win32/debug" /libpath:"../libogg-1.1.3" /libpath:"../libvorbis-1.1.2" /libpath:"../iconv-win32/static" /libpath:"..\win32\glib-2.12.12\lib"

!ENDIF 

# Begin Target

# Name "plugin_exe - Win32 Release"
# Name "plugin_exe - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\dock.c
# End Source File
# Begin Source File

SOURCE=.\options.c
# End Source File
# Begin Source File

SOURCE=.\plugin_main.c
# End Source File
# Begin Source File

SOURCE=.\registry.c
# End Source File
# Begin Source File

SOURCE=.\render.c
# End Source File
# Begin Source File

SOURCE=.\Script.rc
# End Source File
# Begin Source File

SOURCE=.\winamp_exe.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\dock.h
# End Source File
# Begin Source File

SOURCE=.\gen.h
# End Source File
# Begin Source File

SOURCE=.\ipc_pe.h
# End Source File
# Begin Source File

SOURCE=.\options.h
# End Source File
# Begin Source File

SOURCE=.\plugin_main.h
# End Source File
# Begin Source File

SOURCE=.\registry.h
# End Source File
# Begin Source File

SOURCE=.\render.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\wa_ipc.h
# End Source File
# Begin Source File

SOURCE=.\wa_msgids.h
# End Source File
# Begin Source File

SOURCE=.\winamp_exe.h
# End Source File
# Begin Source File

SOURCE=.\wstreamripper.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\sricon.ico
# End Source File
# End Group
# Begin Group "sripper"

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

SOURCE=..\lib\prefs.c
# End Source File
# Begin Source File

SOURCE=..\lib\prefs.h
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

SOURCE=..\lib\utf8.c
# End Source File
# Begin Source File

SOURCE=..\lib\utf8.h
# End Source File
# End Group
# End Target
# End Project
