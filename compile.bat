@echo off
set "cmake=C:\Program Files\CMake\bin\cmake.exe"
call "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvarsall.bat" x64

:: recreate libs so they can be properly delay loaded with msvc
lib /nologo /def:ffmpeg/lib/avcodec-63.def /machine:x64 /out:ffmpeg/lib/avcodec.lib
lib /nologo /def:ffmpeg/lib/avformat-63.def /machine:x64 /out:ffmpeg/lib/avformat.lib
lib /nologo /def:ffmpeg/lib/avutil-61.def /machine:x64 /out:ffmpeg/lib/avutil.lib

"%cmake%" -E remove_directory build & "%cmake%" -G Ninja -S . -B build & ninja -C build
