:: run sdk2013/src/createallprojects.bat and compile everything to get missing .lib files

:: winget install Microsoft.VisualStudio.BuildTools -v "18.12.1" 
:: Visual Studio 2026 Build tools Modify: Desktop Development C++ & MSVC v143 - VS 2022 C++ x64/x86 build tools (v14.44-17.14)

:: winget install MSYS2.MSYS2
:: run in msys2 ucrt64: pacman -S --needed --noconfirm bash pkg-config coreutils findutils grep sed make diffutils nasm

@echo off
set "ROOT=%~dp0"
set "BC=%ROOT%build\cmake"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
cmake -E remove_directory "%BC%"
cmake -G Ninja -S "%ROOT%." -B "%BC%"
ninja -C "%BC%"
