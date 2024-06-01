# "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 10.0.17763.0 -vcvars_ver=14.16
# cmake -G Ninja -S . -B build && ninja -C build

cmake_minimum_required(VERSION 3.19)

project(Plugin VERSION 0.0.1)

set(CMAKE_BUILD_TYPE RelWithDebInfo)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

set(CMAKE_CXX_COMPILER cl)
set(CMAKE_C_COMPILER cl)
set(CMAKE_CXX_STANDARD 20)
set(HL2SDK_DIR sdk2013/mp/src)
set(SOURCEHOOK_DIR metamod-source/core/sourcehook)
set(SRC_DIR src)

set(PLUGIN_NAME ${PROJECT_NAME})
set(PLUGIN_VERSION ${PROJECT_VERSION})

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git/ORIG_HEAD)
    include(cmake/GetGitRevisionDescription.cmake)
    get_git_head_revision(GIT_REFSPEC GIT_SHA1)
    string(SUBSTRING ${GIT_SHA1} 0 7 GIT_SHA1_SHORT)
    set(PLUGIN_VERSION "${PROJECT_VERSION} (${GIT_SHA1_SHORT})")
endif()

configure_file(${SRC_DIR}/PluginInfo.cpp.in PluginInfo.cpp @ONLY)

# disable C4311 warning for sourcehook
set_source_files_properties(${SOURCEHOOK_DIR}/sourcehook_hookmangen.cpp PROPERTIES COMPILE_FLAGS /wd4311)

add_library(${PROJECT_NAME} SHARED
    ${HL2SDK_DIR}/public/vgui_controls/vgui_controls.cpp
    ${HL2SDK_DIR}/public/tier0/memoverride.cpp

    ${SOURCEHOOK_DIR}/sourcehook.cpp
    ${SOURCEHOOK_DIR}/sourcehook_hookmangen.cpp
    ${SOURCEHOOK_DIR}/sourcehook_impl_chookidman.cpp
    ${SOURCEHOOK_DIR}/sourcehook_impl_chookmaninfo.cpp
    ${SOURCEHOOK_DIR}/sourcehook_impl_cproto.cpp
    ${SOURCEHOOK_DIR}/sourcehook_impl_cvfnptr.cpp

    ${CMAKE_BINARY_DIR}/PluginInfo.cpp
    
    ${SRC_DIR}/base/Plugin.cpp
    ${SRC_DIR}/base/Interfaces.cpp
    ${SRC_DIR}/base/Modules.cpp
    ${SRC_DIR}/base/Sig.cpp

    ${SRC_DIR}/modules/Resolution.cpp
    ${SRC_DIR}/modules/Recorder.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${SRC_DIR}

    ${HL2SDK_DIR}/common
    ${HL2SDK_DIR}/public
    ${HL2SDK_DIR}/public/tier0
    ${HL2SDK_DIR}/public/tier1
    ${HL2SDK_DIR}/game
    ${HL2SDK_DIR}/game/shared

    ${SOURCEHOOK_DIR}
)

target_compile_options(${PROJECT_NAME} PRIVATE
    /FI ${CMAKE_SOURCE_DIR}/${SRC_DIR}/base/Common.h
)

set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")

target_compile_definitions(${PROJECT_NAME} PRIVATE
    _CRT_NO_VA_START_VALIDATION
    SUPPRESS_INVALID_PARAMETER_NO_INFO
    VERSION_SAFE_STEAM_API_INTERFACES
    CLIENT_DLL
    WIN32
    RAD_TELEMETRY_DISABLED
    TF2_SDK
    TF_CLIENT_DLL
    GLOWS_ENABLE
    MOVE_CONSTRUCTOR_SUPPORT
    USES_ECON_ITEMS
    CE_DLL
    COMPILER_MSVC64
    WIN64
    _WINDOWS
    _CRT_DECLARE_NONSTDC_NAMES=1
    _CRT_NONSTDC_NO_DEPRECATE=1
)

target_link_directories(${PROJECT_NAME} PRIVATE
    libs
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    legacy_stdio_definitions
    tier0
    tier1
    tier2
    tier3
    steam_api64
    vstdlib
    mathlib
    vgui_controls
)

add_custom_command(
	TARGET ${PROJECT_NAME}
	POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/mod
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> "${PROJECT_SOURCE_DIR}/mod/addons/${PROJECT_NAME}.dll"
    COMMAND ${CMAKE_COMMAND} -E echo "Plugin { file \"addons/${PROJECT_NAME}\" }" > "${PROJECT_SOURCE_DIR}/mod/addons/${PROJECT_NAME}.vdf"
)