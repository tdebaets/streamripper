; sripperwa3.nsi
;
; This script will generate an installer that installs a Winamp plug-in.
; It also puts a license page on, for shits and giggles.
;
; This installer will automatically alert the user that installation was
; successful, and ask them whether or not they would like to make the 
; plug-in the default and run Winamp.
;

; The name of the installer
Name "Streamripper for Winamp3 v1.52"

; The file to write
OutFile "srwa3_152.exe"

; The default installation directory
InstallDir $PROGRAMFILES\Winamp3
; detect winamp path from uninstall string if available
InstallDirRegKey HKLM \
                 "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp3" \
                 "UninstallString"

; The text to prompt the user to enter a directory
DirText "Please select your Winamp path below (you will be able to proceed when Winamp is detected):"
; DirShow hide

; automatically close the installer when done.
AutoCloseWindow true
; hide the "show details" box
; ShowInstDetails nevershow

; this is probably buggy
Function QueryWinampWacPath ; sets $1 with vis path
  StrCpy $1 $INSTDIR\Wacs
FunctionEnd

; The stuff to install
Section "ThisNameIsIgnoredSoWhyBother?"

  Call QueryWinampWacPath
  SetOutPath $1

  ; File to extract
  File "C:\program files\winamp3\wacs\sripper.wac"

  ; prompt user, and if they select no, skip the following 3 instructions.
  MessageBox MB_YESNO|MB_ICONQUESTION \
             "The plug-in was installed. Would you like to run Winamp3 now? (you will find it in the thinger bar)" \
             IDNO NoWinamp
    Exec '"$INSTDIR\Studio.exe"'
  NoWinamp:
SectionEnd

; eof
