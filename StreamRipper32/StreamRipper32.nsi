Name "StreamRipper32"
ComponentText "This will install StreamRipper32"
DirText "Setup has determined the optimal location to install. If you would like to change the directory, do so now."
LicenseText "StreamRipper32 in under the GNU Public License...Please read the license terms below before installing."
LicenseData Doc/COPYING
OutFile StreamRipper32_2_2.exe
InstallDir $PROGRAMFILES\StreamRipper32
InstType Normal
Section "StreamRipper32"
SectionIn 1
SetOutPath $INSTDIR
File Release\StreamRipper32.exe
File Doc\streamripper32.html
File Doc\Streamripper32Logo.jpg
File Doc\AUTHORS
File Doc\COPYING
File Doc\INSTALL
File Doc\Readme
Section -post
Exec '"explorer" "$INSTDIR\streamripper32.html"'
