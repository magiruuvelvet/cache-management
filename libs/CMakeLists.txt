####################################################################################
####################################################################################
#####                                                                          #####
##### ONLY INTRODUCE DEPENDENCIES WHICH HAVE A PERMISSIVE NON-COPYLEFT LICENSE #####
#####                                                                          #####
####################################################################################
####################################################################################

# rapidyaml (MIT)
add_subdirectory(rapidyaml EXCLUDE_FROM_ALL SYSTEM)

# Catch2 (BSL-1.0)
option(CATCH_INSTALL_DOCS "" OFF)
option(CATCH_INSTALL_EXTRAS "" OFF)
option(CATCH_DEVELOPMENT_BUILD "" OFF)
add_subdirectory(Catch2 EXCLUDE_FROM_ALL SYSTEM)
if (ENABLE_PRECOMPILED_HEADERS)
    target_precompile_headers(Catch2 PUBLIC
        "$<$<COMPILE_LANGUAGE:CXX>:<catch2/catch_test_macros.hpp$<ANGLE-R>>"
    )
endif()

# {fmt} (MIT)
option(FMT_INSTALL "" OFF)
option(FMT_SYSTEM_HEADERS "" ON)
add_subdirectory(fmt EXCLUDE_FROM_ALL SYSTEM)
if (ENABLE_PRECOMPILED_HEADERS)
    target_precompile_headers(fmt PUBLIC
        "$<$<COMPILE_LANGUAGE:CXX>:<fmt/core.h$<ANGLE-R>>"
        "$<$<COMPILE_LANGUAGE:CXX>:<fmt/format.h$<ANGLE-R>>"
        "$<$<COMPILE_LANGUAGE:CXX>:<fmt/ostream.h$<ANGLE-R>>"
    )
endif()

# quill (MIT)
option(QUILL_FMT_EXTERNAL "" ON)
option(QUILL_NO_EXCEPTIONS "" OFF) # FIXME: disable exceptions
add_subdirectory(quill EXCLUDE_FROM_ALL SYSTEM)
if (ENABLE_PRECOMPILED_HEADERS)
    target_precompile_headers(quill PUBLIC
        "$<$<COMPILE_LANGUAGE:CXX>:<quill/Quill.h$<ANGLE-R>>"
    )
endif()

# argparse-cpp (MIT)
add_subdirectory(argparse-cpp/src EXCLUDE_FROM_ALL SYSTEM)

# simdjson (Apache-2.0)
set(SIMDJSON_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/simdjson/include")
add_library(3rdparty_simdjson STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/simdjson/src/simdjson.cpp"
    "${SIMDJSON_INCLUDE_DIR}/simdjson.h")
target_include_directories(3rdparty_simdjson PRIVATE "${SIMDJSON_INCLUDE_DIR}")
target_include_directories(3rdparty_simdjson PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/simdjson/src")
# use generic fallback implementation until there is proper runtime cpu detection support
target_compile_definitions(3rdparty_simdjson PRIVATE "-DSIMDJSON_IMPLEMENTATION_FALLBACK")
target_compile_definitions(3rdparty_simdjson PRIVATE SIMDJSON_EXCEPTIONS=OFF)
set_property(TARGET 3rdparty_simdjson PROPERTY CXX_STANDARD 20)
set_property(TARGET 3rdparty_simdjson PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET 3rdparty_simdjson PROPERTY CXX_EXTENSIONS OFF)
target_compile_options(3rdparty_simdjson PRIVATE -fno-exceptions -fno-rtti)
add_library(simdjson INTERFACE)
target_link_libraries(simdjson INTERFACE 3rdparty_simdjson)
target_include_directories(simdjson SYSTEM INTERFACE "${SIMDJSON_INCLUDE_DIR}")
target_compile_definitions(simdjson INTERFACE SIMDJSON_EXCEPTIONS=OFF)

# sqlite3 (Public Domain)
if (USE_SYSTEM_SQLITE)
    find_package(SQLite3 "3.43.0" REQUIRED)
    add_library(sqlite3 INTERFACE)
    target_link_libraries(sqlite3 INTERFACE "${SQLite3_LIBRARIES}")
    target_include_directories(sqlite3 SYSTEM INTERFACE "${SQLite3_INCLUDE_DIRS}")
else()
    set(SQLITE3_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sqlite3")
    add_library(3rdparty_sqlite3 STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/sqlite3/sqlite3.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/sqlite3/sqlite3.h")
    target_include_directories(3rdparty_sqlite3 PRIVATE "${SQLITE3_INCLUDE_DIR}")
    set_property(TARGET 3rdparty_sqlite3 PROPERTY C_STANDARD 99)
    set_property(TARGET 3rdparty_sqlite3 PROPERTY C_STANDARD_REQUIRED ON)
    set_property(TARGET 3rdparty_sqlite3 PROPERTY C_EXTENSIONS OFF)
    add_library(sqlite3 INTERFACE)
    target_link_libraries(sqlite3 INTERFACE 3rdparty_sqlite3)
    target_include_directories(sqlite3 SYSTEM INTERFACE "${SQLITE3_INCLUDE_DIR}")
endif()
