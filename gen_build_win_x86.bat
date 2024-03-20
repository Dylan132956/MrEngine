@echo off
if not exist build\win_x86 (
    mkdir build\win_x86
)
cd build\win_x86
cmake ..\..\ -G "Visual Studio 16 2019" -A Win32 -DTarget=Windows
cd ..\..\
pause