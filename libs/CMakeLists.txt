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

# {fmt} (MIT)
option(FMT_INSTALL "" OFF)
option(FMT_SYSTEM_HEADERS "" ON)
add_subdirectory(fmt EXCLUDE_FROM_ALL SYSTEM)

# quill (MIT)
option(QUILL_FMT_EXTERNAL "" ON)
option(QUILL_NO_EXCEPTIONS "" OFF) # FIXME: disable exceptions
add_subdirectory(quill EXCLUDE_FROM_ALL SYSTEM)

# argparse-cpp (MIT)
add_subdirectory(argparse-cpp/src EXCLUDE_FROM_ALL SYSTEM)
