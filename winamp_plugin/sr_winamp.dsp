# Microsoft Developer Studio Project File - Name="sr_winamp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=sr_winamp - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "sr_winamp.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sr_winamp.mak" CFG="sr_winamp - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sr_winamp - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "sr_winamp - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "sr_winamp - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SR_WINAMP_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../xaudio_sdk/include" /I "../lib" /I "../libmad" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SR_WINAMP_EXPORTS" /D "USE_LAYER_2" /D "USE_LAYER_1" /D "HAVE_MPGLIB" /D "NOANALYSIS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib ws2_32.lib comctl32.lib libmad.lib /nologo /dll /machine:I386 /out:"c:\Program Files\Winamp\Plugins\gen_sripper.dll" /libpath:"../libmad/release"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=del "C:\Program Files\Winamp\Plugins\gen_sripperd.dll"
# End Special Build Tool

!ELSEIF  "$(CFG)" == "sr_winamp - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SR_WINAMP_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "../lib" /I "../libmad" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "HAVE_MPGLIB" /D "NOANALYSIS" /D "HAVE_MEMCPY" /D "DEBUG_TO_FILE" /D "_DEBUG_" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib ws2_32.lib comctl32.lib libmad.lib /nologo /dll /debug /machine:I386 /out:"C:\Program Files\Winamp\Plugins\gen_sripperd.dll" /pdbtype:sept /libpath:"../libmad/debug"
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=del "C:\Program Files\Winamp\Plugins\gen_sripper.dll"
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "sr_winamp - Win32 Release"
# Name "sr_winamp - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\dock.c
# End Source File
# Begin Source File

SOURCE=.\dsp_sripper.c
# End Source File
# Begin Source File

SOURCE=.\options.c
# End Source File
# Begin Source File

SOURCE=.\render.c
# End Source File
# Begin Source File

SOURCE=.\Script.rc
# End Source File
# Begin Source File

SOURCE=.\winamp.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\dock.h
# End Source File
# Begin Source File

SOURCE=.\dsp_sripper.h
# End Source File
# Begin Source File

SOURCE=.\gen.h
# End Source File
# Begin Source File

SOURCE=..\mpglib\interface.h
# End Source File
# Begin Source File

SOURCE=.\options.h
# End Source File
# Begin Source File

SOURCE=.\render.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\winamp.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\..\..\sripper\winamp_plugin\bob.ico
# End Source File
# Begin Source File

SOURCE=.\bob.ico
# End Source File
# Begin Source File

SOURCE=.\bob_old.ico
# End Source File
# Begin Source File

SOURCE=.\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\icon2.ico
# End Source File
# Begin Source File

SOURCE=.\sr.ico
# End Source File
# Begin Source File

SOURCE=.\sricon.ico
# End Source File
# End Group
# Begin Group "sripper"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\lib\cbuffer.c
# End Source File
# Begin Source File

SOURCE=..\lib\cbuffer.h
# End Source File
# Begin Source File

SOURCE=..\lib\compat.h
# End Source File
# Begin Source File

SOURCE=..\lib\debug.c
# End Source File
# Begin Source File

SOURCE=..\lib\debug.h
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

SOURCE=..\lib\inet.c
# End Source File
# Begin Source File

SOURCE=..\lib\inet.h
# End Source File
# Begin Source File

SOURCE=..\lib\mpeg.c
# End Source File
# Begin Source File

SOURCE=..\lib\mpeg.h
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

SOURCE=..\lib\ripshout.c
# End Source File
# Begin Source File

SOURCE=..\lib\ripshout.h
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

SOURCE=..\lib\threadlib.c
# End Source File
# Begin Source File

SOURCE=..\lib\threadlib.h
# End Source File
# Begin Source File

SOURCE=..\lib\types.h
# End Source File
# Begin Source File

SOURCE=..\lib\util.c
# End Source File
# Begin Source File

SOURCE=..\lib\util.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\gen_sripper.manifest
# End Source File
# End Target
# End Project
