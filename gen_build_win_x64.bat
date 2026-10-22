@echo off
if not exist build\win_x64 (
    mkdir build\win_x64
)
cd build\win_x64
cmake ..\..\ -G "Visual Studio 17 2022" -A x64 -DTarget=Windows -DArch=x64
cd ..\..\
pause