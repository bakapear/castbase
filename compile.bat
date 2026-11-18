@echo off

set "ROOT=%~dp0"
set "MSYS=C:\msys64"
set "SOURCE=%ROOT%ffmpeg"
set "BUILD=%ROOT%build\ffmpeg\build"
set "PREFIX=%ROOT%build\ffmpeg\out"
set "VULKAN=%VULKAN_SDK%\include"
set "CMAKE=C:\Program Files\CMake\bin\cmake.exe"
set "BC=%ROOT%build\cmake"

if not exist "%MSYS%\usr\bin\nasm.exe" "%MSYS%\usr\bin\bash.exe" -lc "pacman -S --needed --noconfirm bash coreutils findutils grep sed make diffutils nasm"

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
if exist "%PREFIX%\lib\avcodec.lib" goto plugin

echo Building FFmpeg...
set "PATH=%MSYS%\usr\bin;%PATH%"
if not exist "%BUILD%" mkdir "%BUILD%"
pushd "%SOURCE%"
bash.exe --noprofile --norc -c "./configure --prefix='%PREFIX:\=/%' --toolchain=msvc --target-os=win64 --arch=x86_64 --x86asmexe=C:/msys64/usr/bin/nasm.exe --enable-static --disable-shared --disable-everything --disable-autodetect --disable-network --disable-programs --disable-doc --disable-avdevice --disable-avfilter --disable-swscale --disable-swresample --enable-avcodec --enable-avformat --enable-avutil --enable-vulkan --extra-cflags=-I'%VULKAN:\=/%' --enable-encoder=h264_vulkan --enable-encoder=hevc_vulkan --enable-encoder=av1_vulkan --enable-encoder=aac --enable-muxer=mp4 --enable-protocol=file"
make.exe -j%NUMBER_OF_PROCESSORS%
make.exe install
popd

:plugin
echo Building Plugin...
"%CMAKE%" -E remove_directory "%BC%"
"%CMAKE%" -G Ninja -S "%ROOT%." -B "%BC%"
ninja -C "%BC%"
