cmake_minimum_required(VERSION 4.0)

project(castbase VERSION 0.0.1)

set(CMAKE_BUILD_TYPE RelWithDebInfo)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

set(CMAKE_CXX_STANDARD 17) # has to be 17 due to C2666 error in utlsymbol.h
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(HL2SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sdk2013/src)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(PLUGIN_NAME ${PROJECT_NAME})
set(PLUGIN_VERSION ${PROJECT_VERSION})

find_package(Vulkan REQUIRED)

find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short=7 HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_SHA1_SHORT OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
    )
    if(GIT_SHA1_SHORT)
        set(PLUGIN_VERSION "${PROJECT_VERSION} (${GIT_SHA1_SHORT})")
    endif()
endif()

configure_file(${SRC_DIR}/PluginInfo.cpp.in ${CMAKE_BINARY_DIR}/PluginInfo.cpp @ONLY)

file(GLOB_RECURSE MODULE_SOURCES "${SRC_DIR}/modules/*.cpp")

add_library(${PROJECT_NAME} SHARED
    ${HL2SDK_DIR}/public/vgui_controls/vgui_controls.cpp

    ${CMAKE_BINARY_DIR}/PluginInfo.cpp

    ${SRC_DIR}/base/Plugin.cpp
    ${SRC_DIR}/base/Interfaces.cpp
    ${SRC_DIR}/base/Modules.cpp
    ${SRC_DIR}/base/Sig.cpp

    ${MODULE_SOURCES}
)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ffmpeg.cmake)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

target_include_directories(${PROJECT_NAME} PRIVATE
    ${SRC_DIR}

    ${HL2SDK_DIR}/common
    ${HL2SDK_DIR}/public
    ${HL2SDK_DIR}/public/tier0
    ${HL2SDK_DIR}/public/tier1
    ${HL2SDK_DIR}/game
    ${HL2SDK_DIR}/game/shared

    ${Vulkan_INCLUDE_DIRS}
    ${FFMPEG_INCLUDE_DIR}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    CLIENT_DLL
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

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /FI ${SRC_DIR}/base/Common.h
        /W4 /permissive- /GL
    )

    set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")

    target_sources(${PROJECT_NAME} PRIVATE ${HL2SDK_DIR}/public/tier0/memoverride.cpp)
    set_source_files_properties(
        ${HL2SDK_DIR}/public/tier0/memoverride.cpp
        PROPERTIES
            COMPILE_DEFINITIONS "_DEBUG;_ITERATOR_DEBUG_LEVEL=0;_ALLOW_RUNTIME_LIBRARY_MISMATCH"
            COMPILE_OPTIONS "/FI${SRC_DIR}/base/fills/NoDbgAssert.h"
    )

    target_compile_definitions(${PROJECT_NAME} PRIVATE COMPILER_MSVC COMPILER_MSVC64)
    target_link_directories(${PROJECT_NAME} PRIVATE ${HL2SDK_DIR}/lib/public/x64)

    target_link_libraries(${PROJECT_NAME} PRIVATE
        tier0 tier1 tier2 tier3 steam_api64 vstdlib mathlib vgui_controls
        ${Vulkan_LIBRARIES}
        ${AVFORMAT_LIB} ${AVCODEC_LIB} ${AVUTIL_LIB} bcrypt
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        "SHELL:-include ${SRC_DIR}/base/Common.h"
        -Wall -Wno-format-security -Wno-invalid-offsetof -fpermissive -fno-strict-aliasing -msse2 -mfpmath=sse
    )

    target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
        ${HL2SDK_DIR}/common
        ${HL2SDK_DIR}/public
        ${HL2SDK_DIR}/public/tier0
        ${HL2SDK_DIR}/public/tier1
        ${HL2SDK_DIR}/game
        ${HL2SDK_DIR}/game/shared
    )

    target_compile_definitions(${PROJECT_NAME} PRIVATE COMPILER_GCC POSIX _POSIX LINUX _LINUX GNUC NO_MALLOC_OVERRIDE NO_HOOK_MALLOC)
    target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++ -Wl,--exclude-libs,ALL)

    set_target_properties(${PROJECT_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)

    set(SDKLIB_DIR ${HL2SDK_DIR}/lib/public/linux64)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${SDKLIB_DIR}/libtier0.so
        ${SDKLIB_DIR}/libvstdlib.so
        ${SDKLIB_DIR}/libsteam_api.so
        pthread dl m
    )

    # order-sensitive on gnu
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "$<LINK_GROUP:RESCAN,${SDKLIB_DIR}/tier3.a,${SDKLIB_DIR}/tier2.a,${SDKLIB_DIR}/tier1.a,${SDKLIB_DIR}/mathlib.a,${SDKLIB_DIR}/vgui_controls.a>"
        "$<LINK_GROUP:RESCAN,${AVFORMAT_LIB},${AVCODEC_LIB},${AVUTIL_LIB}>"
        ${Vulkan_LIBRARIES}
    )
endif()

set(DIST_ADDONS "${CMAKE_SOURCE_DIR}/dist/tf/addons")
file(MAKE_DIRECTORY "${DIST_ADDONS}")
file(WRITE "${DIST_ADDONS}/${PROJECT_NAME}.vdf" "Plugin { file \"addons/${PROJECT_NAME}\" }\n")
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> "${DIST_ADDONS}/"
)
