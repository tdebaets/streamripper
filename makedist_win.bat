set ver=1.61.2
set bdir=d:\streamripper-win32-%ver%
deltree /y %bdir%
mkdir %bdir%
copy README %bdir%
copy CHANGES %bdir%
copy COPYING %bdir%
copy THANKS %bdir%
@rem copy iconv-win32\dll\*.dll %bdir%
copy tre-0.7.0\win32\Release\*.dll %bdir%
copy consolewin32\release\sripper.exe %bdir%\streamripper.exe
zip -9 -r \streamripper-win32-%ver%.zip %bdir%
deltree /y %bdir%
