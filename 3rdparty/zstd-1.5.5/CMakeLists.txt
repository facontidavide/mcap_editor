# ################################################################
# Copyright (c) Meta Platforms, Inc. and affiliates.
# All rights reserved.
#
# This source code is licensed under both the BSD-style license (found in the
# LICENSE file in the root directory of this source tree) and the GPLv2 (found
# in the COPYING file in the root directory of this source tree).
# ################################################################

project(libzstd C ASM)

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)
option(ZSTD_BUILD_COMPRESSION "BUILD COMPRESSION MODULE" ON)
option(ZSTD_BUILD_DECOMPRESSION "BUILD DECOMPRESSION MODULE" ON)
option(ZSTD_BUILD_DICTBUILDER "BUILD DICTBUILDER MODULE" ON)

set(ZSTDLIB_VISIBLE "" CACHE STRING "Visibility for ZSTDLIB API")
set(ZSTDERRORLIB_VISIBLE "" CACHE STRING "Visibility for ZSTDERRORLIB_VISIBLE API")
set(ZDICTLIB_VISIBLE "" CACHE STRING "Visibility for ZDICTLIB_VISIBLE API")
set(ZSTDLIB_STATIC_API "" CACHE STRING "Visibility for ZSTDLIB_STATIC_API API")
set(ZDICTLIB_STATIC_API "" CACHE STRING "Visibility for ZDICTLIB_STATIC_API API")

set_property(CACHE ZSTDLIB_VISIBLE PROPERTY STRINGS "" "hidden" "default" "protected" "internal")
set_property(CACHE ZSTDERRORLIB_VISIBLE PROPERTY STRINGS "" "hidden" "default" "protected" "internal")
set_property(CACHE ZDICTLIB_VISIBLE PROPERTY STRINGS "" "hidden" "default" "protected" "internal")
set_property(CACHE ZSTDLIB_STATIC_API PROPERTY STRINGS "" "hidden" "default" "protected" "internal")
set_property(CACHE ZDICTLIB_STATIC_API PROPERTY STRINGS "" "hidden" "default" "protected" "internal")

set(Sources src/zstd.c)

# Explicitly set the language to C for all files, including ASM files.
# Our assembly expects to be compiled by a C compiler, and is only enabled for
# __GNUC__ compatible compilers. Otherwise all the ASM code is disabled by
# macros.
if(NOT CMAKE_ASM_COMPILER STREQUAL CMAKE_C_COMPILER)
    set_source_files_properties(${Sources} PROPERTIES LANGUAGE C)
endif()

macro (add_definition target var)
    if (NOT ("${${var}}" STREQUAL ""))
        set_property(TARGET ${target} APPEND PROPERTY COMPILE_DEFINITIONS "${var}=__attribute__((visibility(\"${${var}}\")))")
    endif ()
endmacro ()

# Split project to static and shared libraries build
set(library_targets)

    add_library(libzstd_static STATIC ${Sources} ${Headers})
    list(APPEND library_targets libzstd_static)
    if (ZSTD_MULTITHREAD_SUPPORT)
        set_property(TARGET libzstd_static APPEND PROPERTY COMPILE_DEFINITIONS "ZSTD_MULTITHREAD")
        if (UNIX)
            target_link_libraries(libzstd_static ${THREADS_LIBS})
        endif ()
    endif ()
    add_definition(libzstd_static ZSTDLIB_VISIBLE)
    add_definition(libzstd_static ZSTDERRORLIB_VISIBLE)
    add_definition(libzstd_static ZDICTLIB_VISIBLE)
    add_definition(libzstd_static ZSTDLIB_STATIC_API)
    add_definition(libzstd_static ZDICTLIB_STATIC_API)

# Add specific compile definitions for MSVC project
if (MSVC)
    set_property(TARGET libzstd_static APPEND PROPERTY COMPILE_DEFINITIONS "ZSTD_HEAPMODE=0;_CRT_SECURE_NO_WARNINGS")
endif ()

# With MSVC static library needs to be renamed to avoid conflict with import library
if (MSVC OR (WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT MINGW))
    set(STATIC_LIBRARY_BASE_NAME zstd_static)
else ()
    set(STATIC_LIBRARY_BASE_NAME zstd)
endif ()

# Define static and shared library names
set_target_properties(
            libzstd_static
            PROPERTIES
            POSITION_INDEPENDENT_CODE On
            OUTPUT_NAME ${STATIC_LIBRARY_BASE_NAME})

