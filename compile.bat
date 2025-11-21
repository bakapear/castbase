@echo off
call "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvarsall.bat" x64
cmake -E remove_directory build & cmake -G Ninja -S . -B build & ninja -C build
