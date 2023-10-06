# note on 3.25 being the minimum requirement: add_subdirectory() with SYSTEM support
# see https://cmake.org/cmake/help/latest/command/add_subdirectory.html
cmake_minimum_required(VERSION 3.25)

# create options here (everything at a centralized location)
option(ENABLE_GIT_VERSION_TRACKING "embed live git version information" ON)

# process early option dependencies here
if (ENABLE_GIT_VERSION_TRACKING)
    find_package(Git)
endif()

# register custom cmake modules
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")
include(DetectAndRegisterPlatformMacros)
include(DetermineCompiler)
include(GitVersionTracking)
include(SetupTarget)

project("cache-manager" LANGUAGES CXX)

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

# create dependency targets
add_subdirectory(libs)

# add project targets
add_subdirectory(src/utils)
add_subdirectory(src/libcachemgr)
add_subdirectory(src/cachemgr)
add_subdirectory(test)
