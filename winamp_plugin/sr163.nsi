; sr163.nsi
; Modified for version 1.63

;---------------------
;Header files

  !include "MUI.nsh"
  !include "InstallOptions.nsh"
  !include "FileFunc.nsh"
  !insertmacro GetParent
  !insertmacro DirState

;--------------------------------
;General

  ;Name and file
  Name "Streamripper for Windows and Winamp v1.63-beta-2"
  OutFile "streamripper-windows-bundle-1.63-beta-2.exe"

  ;Default installation folder
  InstallDir "$PROGRAMFILES\Streamripper-1.63-beta-2"
  ;InstallDir "$PROGRAMFILES\Streamripper"
  
  ;Get installation folder from registry if available
  ; InstallDirRegKey HKCU "Software\Streamripper" ""


;--------------------------------
;Pages

;  !insertmacro MUI_PAGE_LICENSE "${NSISDIR}\Docs\Modern UI\License.txt"
  !insertmacro MUI_PAGE_COMPONENTS
  Page custom CustomPage
;  Page custom CustomPageB
;  !insertmacro MUI_PAGE_DIRECTORY
;  Page custom CustomPageC
  !insertmacro MUI_PAGE_INSTFILES
  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  
;--------------------------------
;Interface Settings

;  !define MUI_ABORTWARNING
  
;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Reserve Files
  
  ;If you are using solid compression, files that are required before
  ;the actual installation should be stored first in the data block,
  ;because this will make your installer start faster.
  
  ReserveFile "${NSISDIR}\Plugins\InstallOptions.dll"

  ReserveFile "sr163_directories.ini"

;--------------------------------
;Variables

  Var IniValue
  Var WADIR
  Var WADIR_TMP

;--------------------------------
;Installer Sections


Section "Core Libraries" sec_core

  SetOutPath "$INSTDIR"
  
  File "..\README"
  File "..\COPYING"
  File "..\CHANGES"
  File "..\THANKS"
  File "sripper_howto.txt"
  SetOverwrite off
  File "..\parse_rules.txt"
  SetOverwrite on
  File "..\fake_external_metadata.pl"
  File "..\fetch_external_metadata.pl"
  File "D:\sripper_1x\tre-0.7.2\win32\Release\tre.dll"
  File "D:\sripper_1x\libogg-1.1.3\ogg.dll"
  File "D:\sripper_1x\libvorbis-1.1.2\vorbis.dll"
  File "D:\sripper_1x\win32\glib-2.12.12\bin\libglib-2.0-0.dll"
  File "D:\sripper_1x\iconv-win32\dll\iconv.dll"
  File "D:\sripper_1x\intl.dll"
  
  ;Store installation folder(s)
  WriteRegStr HKCU "Software\Streamripper" "" $INSTDIR
  
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  
SectionEnd

Section "Winamp Plugin" sec_plugin

  SetOutPath "$INSTDIR"
  
  File "C:\program files\winamp\plugins\gen_sripper.dll"

  SetOutPath "$WADIR\Plugins"

  File "C:\program files\winamp\plugins\gen_sripper.dll"
  
  ;Store installation folder
  WriteRegStr HKCU "Software\Streamripper" "" $INSTDIR
  
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  
  ;Read a value from an InstallOptions INI file
  !insertmacro INSTALLOPTIONS_READ $IniValue "sr163_directories.ini" "Field 2" "State"
  
SectionEnd

Section "GUI Application" sec_gui

  SetOutPath "$INSTDIR"
  File "C:\Program Files\Streamripper\wstreamripper.exe"

  SetOutPath $INSTDIR\Skins
  File srskin.bmp
  File srskin_winamp.bmp
  File srskin_XP.bmp
  
  ;Store installation folder
  WriteRegStr HKCU "Software\Streamripper" "" $INSTDIR
  
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  
SectionEnd

Section "Console Application" sec_console

  SetOutPath "$INSTDIR"

  File "D:\sripper_1x\consoleWin32\Release\streamripper.exe"
  
  ;Store installation folder
  WriteRegStr HKCU "Software\Streamripper" "" $INSTDIR
  
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  
SectionEnd

Section "Uninstall"
  Delete $WADIR\Plugins\gen_sripper.dll
  Delete $INSTDIR\streamripper.exe
  Delete $INSTDIR\wstreamripper.exe
  Delete $INSTDIR\gen_sripper.dll
  Delete $INSTDIR\iconv.dll
  Delete $INSTDIR\intl.dll
  Delete $INSTDIR\libglib-2.0-0.dll
  Delete $INSTDIR\ogg.dll
  Delete $INSTDIR\tre.dll
  Delete $INSTDIR\vorbis.dll
  Delete $INSTDIR\README
  Delete $INSTDIR\COPYING
  Delete $INSTDIR\THANKS
  Delete $INSTDIR\CHANGES
  Delete $INSTDIR\SRIPPER_HOWTO.TXT
  Delete $INSTDIR\sripper.ini
  Delete $INSTDIR\parse_rules.txt
  Delete $INSTDIR\fake_external_metadata.pl
  Delete $INSTDIR\fetch_external_metadata.pl
  RMDir /r $INSTDIR\Skins
  RMDir $INSTDIR
SectionEnd

;--------------------------------
;Installer Functions

Function .onInit

  ;Extract InstallOptions INI files
  !insertmacro INSTALLOPTIONS_EXTRACT "sr163_directories.ini"

  # set section 'test' as selected and read-only
  IntOp $0 ${SF_SELECTED} | ${SF_RO}
  SectionSetFlags ${sec_core} $0
  SectionSetFlags ${sec_gui} $0

FunctionEnd

Function .onSelChange

  SectionGetFlags ${sec_plugin} $0
  SectionGetFlags ${sec_gui} $1
  IntOp $0 $0 & ${SF_SELECTED}
  IntCmp $0 ${SF_SELECTED} with_plugin no_plugin no_plugin
with_plugin:
  IntOp $1 $1 | ${SF_SELECTED}
  IntOp $1 $1 | ${SF_RO}
  goto set_flags
no_plugin:
  IntOp $2 ${SF_RO} ~
  IntOp $1 $1 & $2
set_flags:
  SectionSetFlags ${sec_gui} $1

FunctionEnd

LangString TEXT_IO_TITLE ${LANG_ENGLISH} "InstallOptions page"
LangString TEXT_IO_SUBTITLE ${LANG_ENGLISH} "This is a page created using the InstallOptions plug-in."

Function CustomPage

  ;; ReadINIStr $0 "$PLUGINSDIR\test.ini" "Field 8" "State"
  ;; WriteINIStr "$PLUGINSDIR\test.ini" "Settings" "RTL" "1"
  ;; !insertmacro INSTALLOPTIONS_READ $IniValue "sr163_directories.ini" "Field 2" "State"

  StrCpy $WADIR $PROGRAMFILES\Winamp

  ; detect winamp path from uninstall string if available
  ; otherwise default to $PROGRAMFILES\Winamp
  ReadRegStr $WADIR_TMP \
                 HKLM \
                 "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp" \
                 "UninstallString"
  IfErrors WADIR_DEFAULT 0
    StrCpy $WADIR_TMP $WADIR_TMP -1 1
    ${GetParent} "$WADIR_TMP" $R0
    ${DirState} "$R0" $R1
    IntCmp $R1 -1 WADIR_DEFAULT
      StrCpy $WADIR $R0
WADIR_DEFAULT:

  SectionGetFlags ${sec_plugin} $0
  IntOp $0 $0 & ${SF_SELECTED}
  IntCmp $0 ${SF_SELECTED} with_plugin
  !insertmacro INSTALLOPTIONS_WRITE "sr163_directories.ini" "Field 3" "Flags" "DISABLED"
  !insertmacro INSTALLOPTIONS_WRITE "sr163_directories.ini" "Field 4" "Flags" "DISABLED"
  goto set_directories
with_plugin:
  !insertmacro INSTALLOPTIONS_WRITE "sr163_directories.ini" "Field 3" "Flags" ""
  !insertmacro INSTALLOPTIONS_WRITE "sr163_directories.ini" "Field 4" "Flags" ""
set_directories:
  !insertmacro INSTALLOPTIONS_WRITE "sr163_directories.ini" "Field 2" "State" $INSTDIR
  !insertmacro INSTALLOPTIONS_WRITE "sr163_directories.ini" "Field 4" "State" $WADIR
  !insertmacro MUI_HEADER_TEXT "$(TEXT_IO_TITLE)" "$(TEXT_IO_SUBTITLE)"
  !insertmacro INSTALLOPTIONS_DISPLAY "sr163_directories.ini"
  !insertmacro INSTALLOPTIONS_READ $INSTDIR "sr163_directories.ini" "Field 2" "State"
  !insertmacro INSTALLOPTIONS_READ $WADIR "sr163_directories.ini" "Field 4" "State"

FunctionEnd

;Function CustomPageB

;  !insertmacro MUI_HEADER_TEXT "$(TEXT_IO_TITLE)" "$(TEXT_IO_SUBTITLE)"
;  !insertmacro INSTALLOPTIONS_DISPLAY "ioB.ini"

;FunctionEnd

;Function CustomPageC

;  !insertmacro MUI_HEADER_TEXT "$(TEXT_IO_TITLE)" "$(TEXT_IO_SUBTITLE)"
;  !insertmacro INSTALLOPTIONS_DISPLAY "sr163_directories.ini"

;FunctionEnd

;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_SecDummy1 ${LANG_ENGLISH} "Streamripper core.  You need this."
  LangString DESC_SecDummy2 ${LANG_ENGLISH} "The Winamp plugin.  This uses the GUI application for actual ripping."
  LangString DESC_SecDummy3 ${LANG_ENGLISH} "The GUI version of streamripper.  You need this for the winamp plugin.  But you can run it standalone too."
  LangString DESC_SecDummy4 ${LANG_ENGLISH} "The streamripper command-line application.  Very handy for advanced users."

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${sec_core} $(DESC_SecDummy1)
    !insertmacro MUI_DESCRIPTION_TEXT ${sec_plugin} $(DESC_SecDummy2)
    !insertmacro MUI_DESCRIPTION_TEXT ${sec_gui} $(DESC_SecDummy3)
    !insertmacro MUI_DESCRIPTION_TEXT ${sec_console} $(DESC_SecDummy4)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  ;ADD YOUR OWN FILES HERE...

  Delete "$INSTDIR\Uninstall.exe"

  RMDir "$INSTDIR"

  DeleteRegKey /ifempty HKCU "Software\Streamripper"

SectionEnd