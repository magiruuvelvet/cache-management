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
target_precompile_headers(Catch2 PUBLIC
    "$<$<COMPILE_LANGUAGE:CXX>:<catch2/catch_test_macros.hpp$<ANGLE-R>>"
)

# {fmt} (MIT)
option(FMT_INSTALL "" OFF)
option(FMT_SYSTEM_HEADERS "" ON)
add_subdirectory(fmt EXCLUDE_FROM_ALL SYSTEM)
target_precompile_headers(fmt PUBLIC
    "$<$<COMPILE_LANGUAGE:CXX>:<fmt/core.h$<ANGLE-R>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<fmt/format.h$<ANGLE-R>>"
    "$<$<COMPILE_LANGUAGE:CXX>:<fmt/ostream.h$<ANGLE-R>>"
)

# quill (MIT)
option(QUILL_FMT_EXTERNAL "" ON)
option(QUILL_NO_EXCEPTIONS "" OFF) # FIXME: disable exceptions
add_subdirectory(quill EXCLUDE_FROM_ALL SYSTEM)
target_precompile_headers(quill PUBLIC
    "$<$<COMPILE_LANGUAGE:CXX>:<quill/Quill.h$<ANGLE-R>>"
)

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
add_library(simdjson INTERFACE)
target_link_libraries(simdjson INTERFACE 3rdparty_simdjson)
target_include_directories(simdjson SYSTEM INTERFACE "${SIMDJSON_INCLUDE_DIR}")
target_compile_definitions(simdjson INTERFACE SIMDJSON_EXCEPTIONS=OFF)
