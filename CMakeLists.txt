# "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
# cmake -G Ninja -S . -B build && ninja -C build

cmake_minimum_required(VERSION 3.19)

project(Plugin VERSION 0.0.1)

set(CMAKE_BUILD_TYPE RelWithDebInfo)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

set(CMAKE_CXX_STANDARD 17) # has to be 17 due to C2666 error in utlsymbol.h
set(HL2SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sdk2013/src)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(PLUGIN_NAME ${PROJECT_NAME})
set(PLUGIN_VERSION ${PROJECT_VERSION})

# Git version fetch
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git/ORIG_HEAD)
    include(cmake/GetGitRevisionDescription.cmake)
    get_git_head_revision(GIT_REFSPEC GIT_SHA1)
    string(SUBSTRING ${GIT_SHA1} 0 7 GIT_SHA1_SHORT)
    set(PLUGIN_VERSION "${PROJECT_VERSION} (${GIT_SHA1_SHORT})")
endif()

# PluginInfo
configure_file(${SRC_DIR}/PluginInfo.cpp.in ${CMAKE_BINARY_DIR}/PluginInfo.cpp @ONLY)

# Plugin source files
add_library(${PROJECT_NAME} SHARED
    ${HL2SDK_DIR}/public/vgui_controls/vgui_controls.cpp
    ${HL2SDK_DIR}/public/tier0/memoverride.cpp

    ${CMAKE_BINARY_DIR}/PluginInfo.cpp

    ${SRC_DIR}/base/Plugin.cpp
    ${SRC_DIR}/base/Interfaces.cpp
    ${SRC_DIR}/base/Modules.cpp
    ${SRC_DIR}/base/Sig.cpp

    ${SRC_DIR}/modules/Resolution.cpp
    ${SRC_DIR}/modules/Recorder.cpp
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${SRC_DIR}

    ${HL2SDK_DIR}/common
    ${HL2SDK_DIR}/public
    ${HL2SDK_DIR}/public/tier0
    ${HL2SDK_DIR}/public/tier1
    ${HL2SDK_DIR}/game
    ${HL2SDK_DIR}/game/shared
)

# Compiler options
target_compile_options(${PROJECT_NAME} PRIVATE
    /FI ${SRC_DIR}/base/Common.h
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
)

# Post-build steps
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/mod
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> "${CMAKE_SOURCE_DIR}/mod/addons/${PROJECT_NAME}.dll"
    COMMAND ${CMAKE_COMMAND} -E echo "Plugin { file \"addons/${PROJECT_NAME}\" }" > "${CMAKE_SOURCE_DIR}/mod/addons/${PROJECT_NAME}.vdf"
)
