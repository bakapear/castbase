# "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
# cmake -G Ninja -S . -B build && ninja -C build

cmake_minimum_required(VERSION 4.0)

project(castbase VERSION 0.0.1)

set(CMAKE_BUILD_TYPE RelWithDebInfo)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

set(CMAKE_CXX_STANDARD 17) # has to be 17 due to C2666 error in utlsymbol.h
set(HL2SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sdk2013/src)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(FFMPEG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg)

set(PLUGIN_NAME ${PROJECT_NAME})
set(PLUGIN_VERSION ${PROJECT_VERSION})

find_package(Vulkan REQUIRED)

find_path(FFMPEG_INCLUDE_DIRS NAMES libavcodec/avcodec.h PATHS ${FFMPEG_DIR}/include)
find_library(AVCODEC_LIB NAMES avcodec PATHS ${FFMPEG_DIR}/lib)
find_library(AVFORMAT_LIB NAMES avformat PATHS ${FFMPEG_DIR}/lib)
find_library(AVUTIL_LIB NAMES avutil PATHS ${FFMPEG_DIR}/lib)
find_library(SWSCALE_LIB NAMES swscale PATHS ${FFMPEG_DIR}/lib)

# Git version fetch
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git/ORIG_HEAD)
    include(cmake/GetGitRevisionDescription.cmake)
    get_git_head_revision(GIT_REFSPEC GIT_SHA1)
    string(SUBSTRING ${GIT_SHA1} 0 7 GIT_SHA1_SHORT)
    set(PLUGIN_VERSION "${PROJECT_VERSION} (${GIT_SHA1_SHORT})")
endif()

# PluginInfo
configure_file(${SRC_DIR}/PluginInfo.cpp.in ${CMAKE_BINARY_DIR}/PluginInfo.cpp @ONLY)

# Plugin module source files
file(GLOB_RECURSE MODULE_SOURCES
    "${SRC_DIR}/modules/*.cpp"
)

# Plugin source files
add_library(${PROJECT_NAME} SHARED
    ${HL2SDK_DIR}/public/vgui_controls/vgui_controls.cpp
    ${HL2SDK_DIR}/public/tier0/memoverride.cpp

    ${CMAKE_BINARY_DIR}/PluginInfo.cpp

    ${SRC_DIR}/base/Plugin.cpp
    ${SRC_DIR}/base/Interfaces.cpp
    ${SRC_DIR}/base/Modules.cpp
    ${SRC_DIR}/base/Sig.cpp

    ${MODULE_SOURCES}
)

# Minhook
add_subdirectory(minhook)
target_link_libraries(${PROJECT_NAME} PRIVATE minhook)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${SRC_DIR}

    ${HL2SDK_DIR}/common
    ${HL2SDK_DIR}/public
    ${HL2SDK_DIR}/public/tier0
    ${HL2SDK_DIR}/public/tier1
    ${HL2SDK_DIR}/game
    ${HL2SDK_DIR}/game/shared

    ${Vulkan_INCLUDE_DIRS}
    ${FFMPEG_INCLUDE_DIRS}
)

# Delay load FFMPEG dlls
target_link_options(${PROJECT_NAME} PRIVATE
    "/DELAYLOAD:avcodec-63.dll"
    "/DELAYLOAD:avformat-63.dll"
    "/DELAYLOAD:avutil-61.dll"
)
target_link_libraries(${PROJECT_NAME} PRIVATE delayimp)

# Compiler options
target_compile_options(${PROJECT_NAME} PRIVATE
    /FI ${SRC_DIR}/base/Common.h

    /W4
    /permissive-
    /GL
)

# Set MSVC runtime library to MultiThreaded
set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")

# Compiler definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    COMPILER_MSVC
    COMPILER_MSVC64
    CLIENT_DLL

    # has to be set so memoverride.cpp can build with latest Windows SDK
    _DEBUG # so that crtdbg.h doesnt redefine malloc
    NDEBUG # undefine debug afterwards

    # useful defines from client_win64_tf.vcxproj
    REPLAY_ENABLED
    NO_STRING_T
    INCLUDED_STEAM2_USERID_STRUCTS
    USES_ECON_ITEMS
    TF_CLIENT_DLL
    ENABLE_GC_MATCHMAKING
    GLOWS_ENABLE
    USE_DYNAMIC_ASSET_LOADING
    VOTING_ENABLED
    NEXT_BOT
    INVENTORY_VIA_WEBAPI
)

# Link directories
target_link_directories(${PROJECT_NAME} PRIVATE
    ${HL2SDK_DIR}/lib/public/x64
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    tier0
    tier1
    tier2
    tier3
    steam_api64
    vstdlib
    mathlib
    vgui_controls
    d3d11
    dxgi

    ${Vulkan_LIBRARIES}

    ${AVCODEC_LIB}
    ${AVFORMAT_LIB}
    ${AVUTIL_LIB}
    ${SWSCALE_LIB}
)

file(GLOB FFMPEG_DLLS "${FFMPEG_DIR}/bin/*.dll")

# Post-build steps
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/mod

    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> "${CMAKE_SOURCE_DIR}/mod/addons/${PROJECT_NAME}.dll"
    COMMAND ${CMAKE_COMMAND} -E echo "Plugin { file \"addons/${PROJECT_NAME}\" }" > "${CMAKE_SOURCE_DIR}/mod/addons/${PROJECT_NAME}.vdf"

    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/mod/addons/ffmpeg"
    COMMAND ${CMAKE_COMMAND} -E copy ${FFMPEG_DLLS} "${CMAKE_SOURCE_DIR}/mod/addons/ffmpeg"
)
