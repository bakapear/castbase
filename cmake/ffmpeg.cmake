get_filename_component(FFMPEG_ROOT "${CMAKE_BINARY_DIR}/../cmake-ffmpeg" ABSOLUTE)
set(FFMPEG_DIR "${FFMPEG_ROOT}/out")
set(FFMPEG_INCLUDE_DIR "${FFMPEG_DIR}/include")
set(AVCODEC_LIB "${FFMPEG_DIR}/lib/avcodec.lib")
set(AVFORMAT_LIB "${FFMPEG_DIR}/lib/avformat.lib")
set(AVUTIL_LIB "${FFMPEG_DIR}/lib/avutil.lib")

if(EXISTS "${AVCODEC_LIB}" AND EXISTS "${AVFORMAT_LIB}" AND EXISTS "${AVUTIL_LIB}")
    add_custom_target(ffmpeg)
else()
    include(ExternalProject)

    set(MSYS_DIR "C:/msys64")
    set(FFMPEG_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg")
    set(FFMPEG_BASH "${MSYS_DIR}/usr/bin/bash.exe")

    if(NOT EXISTS "${MSYS_DIR}/usr/bin/nasm.exe")
        execute_process(
            COMMAND "${FFMPEG_BASH}" -lc "pacman -S --needed --noconfirm bash coreutils findutils grep sed make diffutils nasm"
            COMMAND_ERROR_IS_FATAL ANY
        )
    endif()

    set(FFMPEG_CONFIGURE_ARGS
        --prefix='${FFMPEG_DIR}'
        --toolchain=msvc
        --target-os=win64
        --arch=x86_64
        --x86asmexe='${MSYS_DIR}/usr/bin/nasm.exe'
        --enable-static
        --disable-shared
        --disable-everything
        --disable-autodetect
        --disable-network
        --disable-programs
        --disable-doc
        --disable-avdevice
        --disable-avfilter
        --disable-swscale
        --disable-swresample
        --enable-avcodec
        --enable-avformat
        --enable-avutil
        --enable-vulkan
        --extra-cflags=-I'${Vulkan_INCLUDE_DIRS}'
        --enable-encoder=h264_vulkan
        --enable-encoder=hevc_vulkan
        --enable-encoder=av1_vulkan
        --enable-encoder=aac
        --enable-muxer=mp4
        --enable-protocol=file
    )
    list(JOIN FFMPEG_CONFIGURE_ARGS " " FFMPEG_CONFIGURE_ARGS_STR)

    set(FFMPEG_SH ${FFMPEG_BASH} --noprofile --norc -c)
    set(FFMPEG_ENV "export PATH=/usr/bin:$PATH")

    include(ProcessorCount)
    ProcessorCount(n)

    ExternalProject_Add(ffmpeg
        PREFIX "${FFMPEG_ROOT}"
        SOURCE_DIR "${FFMPEG_SOURCE_DIR}"
        BINARY_DIR "${FFMPEG_ROOT}/build"
        INSTALL_DIR "${FFMPEG_DIR}"
        STAMP_DIR "${FFMPEG_ROOT}/stamp"
        TMP_DIR "${FFMPEG_ROOT}/tmp"
        LOG_DIR "${FFMPEG_ROOT}/log"

        CONFIGURE_COMMAND ${FFMPEG_SH} "${FFMPEG_ENV} && '${FFMPEG_SOURCE_DIR}/configure' ${FFMPEG_CONFIGURE_ARGS_STR}"
        BUILD_COMMAND ${FFMPEG_SH} "${FFMPEG_ENV} && make -j${n}"
        INSTALL_COMMAND ${FFMPEG_SH} "${FFMPEG_ENV} && make install"

        BUILD_BYPRODUCTS "${AVCODEC_LIB}" "${AVFORMAT_LIB}" "${AVUTIL_LIB}"

        USES_TERMINAL_CONFIGURE TRUE
        USES_TERMINAL_BUILD TRUE
        USES_TERMINAL_INSTALL TRUE
    )
endif()
