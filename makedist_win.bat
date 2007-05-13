set ver=1.62-beta-7
set bdir=d:\streamripper-win32-%ver%
deltree /y %bdir%
mkdir %bdir%
copy README %bdir%
copy CHANGES %bdir%
copy COPYING %bdir%
copy THANKS %bdir%
copy parse_rules.txt %bdir%
copy fake_external_metadata.pl %bdir%
copy fetch_external_metadata.pl %bdir%
copy libogg-1.1.3\*.dll %bdir%
copy libvorbis-1.1.2\*.dll %bdir%
@rem copy iconv-win32\dll\*.dll %bdir%
copy tre-0.7.2\win32\Release\*.dll %bdir%
copy consolewin32\release\sripper.exe %bdir%\streamripper.exe
erase \streamripper-win32-%ver%.zip
zip -9 -r \streamripper-win32-%ver%.zip %bdir%
deltree /y %bdir%
