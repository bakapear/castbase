@echo off
set "ROOT=%~dp0"
set "CMAKE=C:\Program Files\CMake\bin\cmake.exe"
set "BC=%ROOT%build\cmake"
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
"%CMAKE%" -E remove_directory "%BC%"
"%CMAKE%" -G Ninja -S "%ROOT%." -B "%BC%"
ninja -C "%BC%"
