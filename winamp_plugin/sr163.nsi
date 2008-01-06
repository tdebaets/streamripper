; sr163.nsi
; Modified for version 1.63

;---------------------
;Header files

  !include "MUI.nsh"
  !include "InstallOptions.nsh"

;--------------------------------
;General

  ;Name and file
  Name "Streamripper for Winamp 2.x/5.x v1.63-beta-2"
  OutFile "srwa5-1.63-beta-2.exe"

  ;Default installation folder
  InstallDir "$PROGRAMFILES\Streamripper"
  
  ;Get installation folder from registry if available
  InstallDirRegKey HKCU "Software\Streamripper" ""

;--------------------------------
;Pages

;  !insertmacro MUI_PAGE_LICENSE "${NSISDIR}\Docs\Modern UI\License.txt"
;  Page custom CustomPageA
  !insertmacro MUI_PAGE_COMPONENTS
;  Page custom CustomPageB
  !insertmacro MUI_PAGE_DIRECTORY
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

  ReserveFile "ioA.ini"
  ReserveFile "ioB.ini"
  ReserveFile "ioC.ini"

;--------------------------------
;Variables

  Var IniValue

;--------------------------------
;Installer Sections


Section "Core Libraries" sec_core

  SetOutPath "$INSTDIR"
  
  File "sripper_howto.txt"
  SetOverwrite off
  File "..\parse_rules.txt"
  SetOverwrite on
  File "..\fake_external_metadata.pl"
  File "..\fetch_external_metadata.pl"
  ;ADD YOUR OWN FILES HERE...
  
  ;Store installation folder
  WriteRegStr HKCU "Software\Modern UI Test" "" $INSTDIR
  
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  
  ;Read a value from an InstallOptions INI file
  !insertmacro INSTALLOPTIONS_READ $IniValue "ioC.ini" "Field 2" "State"
  
  ;Display a messagebox if check box was checked
  StrCmp $IniValue "1" "" +2
    MessageBox MB_OK "You checked the check box, here is the MessageBox..."

SectionEnd

Section "Winamp Plugin" sec_plugin

  SetOutPath "$INSTDIR"
  
  File "C:\program files\winamp\plugins\gen_sripper.dll"

  ;ADD YOUR OWN FILES HERE...
  
  ;Store installation folder
  WriteRegStr HKCU "Software\Modern UI Test" "" $INSTDIR
  
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  
  ;Read a value from an InstallOptions INI file
  !insertmacro INSTALLOPTIONS_READ $IniValue "ioC.ini" "Field 2" "State"
  
  ;Display a messagebox if check box was checked
  StrCmp $IniValue "1" "" +2
    MessageBox MB_OK "You checked the check box, here is the MessageBox..."

SectionEnd

Section "GUI Application" sec_gui

  SetOutPath "$INSTDIR"

  SetOutPath $INSTDIR\Skins\SrSkins
  File srskin.bmp
  File srskin_winamp.bmp
  File srskin_XP.bmp
  
  ;Store installation folder
  WriteRegStr HKCU "Software\Modern UI Test" "" $INSTDIR
  
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  
  ;Read a value from an InstallOptions INI file
  !insertmacro INSTALLOPTIONS_READ $IniValue "ioC.ini" "Field 2" "State"
  
SectionEnd

Section "Console Application" sec_console

  SetOutPath "$INSTDIR"
  
  ;Store installation folder
  WriteRegStr HKCU "Software\Modern UI Test" "" $INSTDIR
  
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  
  ;Read a value from an InstallOptions INI file
  !insertmacro INSTALLOPTIONS_READ $IniValue "ioC.ini" "Field 2" "State"
  
  ;Display a messagebox if check box was checked
  StrCmp $IniValue "1" "" +2
    MessageBox MB_OK "You checked the check box, here is the MessageBox..."

SectionEnd

Section "Uninstall"
  Delete $INSTDIR\streamripper_uninstall.exe
  Delete $INSTDIR\Plugins\gen_sripper.dll
  Delete $INSTDIR\Plugins\SRIPPER_HOWTO.TXT
  Delete $INSTDIR\Plugins\sripper.ini
  Delete $INSTDIR\Plugins\parse_rules.txt
  Delete $INSTDIR\Plugins\fake_external_metadata.pl
  Delete $INSTDIR\Plugins\fetch_external_metadata.pl
  RMDir /r $INSTDIR\Skins\SrSkins
SectionEnd

;--------------------------------
;Installer Functions

Function .onInit

  ;Extract InstallOptions INI files
  !insertmacro INSTALLOPTIONS_EXTRACT "ioA.ini"
  !insertmacro INSTALLOPTIONS_EXTRACT "ioB.ini"
  !insertmacro INSTALLOPTIONS_EXTRACT "ioC.ini"

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

;Function CustomPageA

;  !insertmacro MUI_HEADER_TEXT "$(TEXT_IO_TITLE)" "$(TEXT_IO_SUBTITLE)"
;  !insertmacro INSTALLOPTIONS_DISPLAY "ioA.ini"

;FunctionEnd

Function CustomPageB

  !insertmacro MUI_HEADER_TEXT "$(TEXT_IO_TITLE)" "$(TEXT_IO_SUBTITLE)"
  !insertmacro INSTALLOPTIONS_DISPLAY "ioB.ini"

FunctionEnd

Function CustomPageC

  !insertmacro MUI_HEADER_TEXT "$(TEXT_IO_TITLE)" "$(TEXT_IO_SUBTITLE)"
  !insertmacro INSTALLOPTIONS_DISPLAY "ioC.ini"

FunctionEnd

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

  DeleteRegKey /ifempty HKCU "Software\Modern UI Test"

SectionEnd
