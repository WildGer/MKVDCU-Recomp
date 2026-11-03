# mkvsdcu - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(mkvsdcu LANGUAGES CXX)

set(MKVSDCU_CODEGEN_MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/mkvsdcu_manifest.toml"
    CACHE FILEPATH "Manifest for the validated local game folder")

# A configure that fails early can leave empty per-config flags in the cache,
# which silently builds the whole recompiled game at -O0 (several times slower).
foreach(_mkvsdcu_lang CXX)
    if(CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT CMAKE_${_mkvsdcu_lang}_FLAGS_RELEASE MATCHES "-O")
        message(WARNING "CMAKE_${_mkvsdcu_lang}_FLAGS_RELEASE was empty; restoring -O3 -DNDEBUG")
        set(CMAKE_${_mkvsdcu_lang}_FLAGS_RELEASE "-O3 -DNDEBUG" CACHE STRING "" FORCE)
    endif()
endforeach()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Sources
set(MKVSDCU_SOURCES
    src/main.cpp
    src/direct_launch.cpp
    src/direct_launch_win.cpp
    src/mkvsdcu_app.cpp
    src/port_menu/port_menu.cpp
    src/port_menu/port_config.cpp
    src/port_menu/settings_catalog.cpp
    src/host_tweaks.cpp
    src/command_ring_backoff.cpp
    src/app_sleep_backoff.cpp
    src/input/controller_filter.cpp
    src/mods/mod_overlay.cpp
    src/mods/menu_quit.cpp
    src/telemetry/frame_telemetry.cpp
    src/telemetry/system_telemetry.cpp
    src/telemetry/perf_overlay.cpp
    src/telemetry/guest_profiler.cpp
)

if(WIN32)
    add_executable(mkvsdcu WIN32 ${MKVSDCU_SOURCES})
else()
    add_executable(mkvsdcu ${MKVSDCU_SOURCES})
endif()

# x86-64-v3 (AVX2, FMA, BMI2, LZCNT, MOVBE) lets the recompiled code inline
# its many std::fma calls and fold big-endian byte swaps into loads and
# stores. It needs an Intel Haswell / AMD Zen (2013+) CPU; src/cpu_check.cpp
# reports a clear error on older ones. sse4.1 keeps the SDK's baseline.
set(MKVSDCU_CPU_LEVEL "x86-64-v3" CACHE STRING "Instruction set for the recompiled game: x86-64-v3 or sse4.1")
set_property(CACHE MKVSDCU_CPU_LEVEL PROPERTY STRINGS x86-64-v3 sse4.1)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64" AND MKVSDCU_CPU_LEVEL STREQUAL "x86-64-v3")
    set(MKVSDCU_USE_X86_64_V3 ON)
    target_sources(mkvsdcu PRIVATE src/cpu_check.cpp)
    # The check must run on any CPU, so it is built for the x86-64 baseline.
    set_source_files_properties(src/cpu_check.cpp PROPERTIES COMPILE_OPTIONS "-march=x86-64")
elseif(NOT MKVSDCU_CPU_LEVEL STREQUAL "sse4.1")
    message(FATAL_ERROR "MKVSDCU_CPU_LEVEL must be x86-64-v3 or sse4.1")
endif()

option(MKVSDCU_D3D_TRACE "Build optional original-renderer D3D migration trace (Windows)" OFF)
if(MKVSDCU_D3D_TRACE)
    if(NOT WIN32)
        message(FATAL_ERROR "MKVSDCU_D3D_TRACE currently requires Windows ReadProcessMemory")
    endif()
    target_sources(mkvsdcu PRIVATE src/telemetry/d3d_trace.cpp)
    target_compile_definitions(mkvsdcu PRIVATE MKVSDCU_D3D_TRACE_BUILD=1)
    target_link_libraries(mkvsdcu PRIVATE bcrypt)
endif()

# This is target-private host plumbing. The plugin remains selectable at
# launch via --gpu_plugin xenos; no UE3 or game code is altered here.
rexglue_setup_target(mkvsdcu GPU_PLUGINS xenos)

if(MKVSDCU_USE_X86_64_V3)
    # The recompiled code is the separate mkvsdcu_recomp object library made
    # by rexglue_setup_target. No contraction: separate guest multiplies and
    # adds stay separately rounded.
    foreach(_mkvsdcu_target mkvsdcu mkvsdcu_recomp)
        if(TARGET ${_mkvsdcu_target})
            target_compile_options(${_mkvsdcu_target} PRIVATE -march=x86-64-v3 -ffp-contract=off)
        endif()
    endforeach()
endif()

if(WIN32)
    # Performance overlay, plus Windows CNG for retail XEX SHA-256 validation.
    target_link_libraries(mkvsdcu PRIVATE pdh dxgi winmm dbghelp bcrypt)

    # rexglue_setup_target copies the FidelityFX DLLs only for in-tree target
    # names; an installed SDK exports them as rex::amd_fidelityfx_*, so the
    # runtime would fail to load without this.
    foreach(_mkvsdcu_fx rex::amd_fidelityfx_dx12 rex::amd_fidelityfx_vk)
        if(TARGET ${_mkvsdcu_fx})
            add_custom_command(TARGET mkvsdcu POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:${_mkvsdcu_fx}> $<TARGET_FILE_DIR:mkvsdcu>
                VERBATIM)
        endif()
    endforeach()
endif()
