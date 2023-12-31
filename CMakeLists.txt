# note on 3.25 being the minimum requirement: add_subdirectory() with SYSTEM support
# see https://cmake.org/cmake/help/latest/command/add_subdirectory.html
cmake_minimum_required(VERSION 3.25)

#######################################################################################
# create options here (everything at a centralized location)                          #
#######################################################################################

# compile live git version tracking into the application
option(ENABLE_GIT_VERSION_TRACKING "embed live git version information" ON)

# disable this option if your toolchain has problems with precompiled headers
option(ENABLE_PRECOMPILED_HEADERS "precompile expensive header files" ON)

# enable this option to build with LTO (might increase build time)
option(ENABLE_LTO "enable link time optimizations" OFF)

# disable this option to skip building tests
option(ENABLE_TESTING "build tests" ON)

# enable this to perform profiling without the quill logging library (profiling quill is extremely slow)
option(COMPILE_PROFILING_BUILD "compile a version optimized for profiling (completely disables logging)" OFF)

# enable this option to build and link against the system shared SQLite library
option(USE_SYSTEM_SQLITE "use shared SQLite from the system instead of bundled one" OFF)

#######################################################################################
# process early option dependencies here                                              #
#######################################################################################

# live git version tracking requires the git command line to be installed
if (ENABLE_GIT_VERSION_TRACKING)
    find_package(Git)
endif()

# enable optimizations for profiling builds
if (COMPILE_PROFILING_BUILD)
    # disable all logging macros to disable logging entirely
    add_compile_definitions(QUILL_ACTIVE_LOG_LEVEL=QUILL_LOG_LEVEL_NONE)
    # add macro to disable log initialization at compile-time
    add_compile_definitions(CACHEMGR_PROFILING_BUILD)
endif()

#######################################################################################

# register custom cmake modules
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")
include(DetectAndRegisterPlatformMacros)
include(DetermineCompiler)
include(GitVersionTracking)
include(SetupTarget)

project("cache-manager" LANGUAGES CXX C)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# determine compiler vendor
DetermineCompiler()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# set target destination for built targets
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# disallow implicit function declarations globally
# if a 3rd party library fails to build with this flag enabled, then this is considered a bug in the library
if (PROJECT_COMPILER_CLANG OR PROJECT_COMPILER_GCC)
    message(STATUS "Enabling compiler flag: -Werror=implicit-function-declaration")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=implicit-function-declaration")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror=implicit-function-declaration")
endif()

# LTO requires a project(), so check this here
if (ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT LTO_SUPPORTED OUTPUT LTO_ERROR)
    if (LTO_SUPPORTED)
        message(STATUS "LTO enabled")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    else()
        message(FATAL_ERROR "ENABLE_LTO=ON set, but LTO is not supported: <${LTO_ERROR}>")
    endif()
endif()

# create dependency targets
add_subdirectory(libs)

# add project targets
add_subdirectory(src/utils)
add_subdirectory(src/libcachemgr)
add_subdirectory(src/cachemgr)

if (ENABLE_TESTING)
    add_subdirectory(test)
endif()
