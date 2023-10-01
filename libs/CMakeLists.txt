# rapidyaml
# add_library(rapidyaml INTERFACE)
# target_include_directories(rapidyaml SYSTEM INTERFACE "${PROJECT_SOURCE_DIR}/libs/rapidyaml/src")
# target_include_directories(rapidyaml SYSTEM INTERFACE "${PROJECT_SOURCE_DIR}/libs/rapidyaml/ext/c4core/src")
add_subdirectory(rapidyaml EXCLUDE_FROM_ALL SYSTEM)

# Catch2
option(CATCH_INSTALL_DOCS "" OFF)
option(CATCH_INSTALL_EXTRAS "" OFF)
option(CATCH_DEVELOPMENT_BUILD "" OFF)
add_subdirectory(Catch2 EXCLUDE_FROM_ALL SYSTEM)
