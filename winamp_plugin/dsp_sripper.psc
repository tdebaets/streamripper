Name Streamripper for Winamp 1.30
Text This will install Nullsoft Streamripper for Winamp 1.30 on your computer
OutFile gen_sripper.exe
SetOutPath $DSPDIR
AddFile \Program Files\Winamp\Plugins\gen_sripper.dll
AddFile HOWTO.txt
AddFile \Program Files\Winamp\Plugins\srskin.bmp

SetOutPath $INSTDIR
Addfile \Program Files\Winamp\winamp.exe.manifest
ExecFile "$WINDIR\notepad.exe" "$DSPDIR\HOWTO.txt"
; Silent
