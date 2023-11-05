# create library
add_library(cachemgr-utils-private STATIC
    freedesktop/os-release.cpp
    freedesktop/os-release.hpp
    freedesktop/xdg_paths.cpp
    freedesktop/xdg_paths.hpp
    datetime_utils.cpp
    datetime_utils.hpp
    fs_utils.cpp
    fs_utils.hpp
    logging_helper.cpp
    logging_helper.hpp
    number_utils.cpp
    number_utils.hpp
    os_utils.cpp
    os_utils.hpp
)

SetupTarget(cachemgr-utils-private "cachemgr-utils")

# create interface
add_library(cachemgr-utils INTERFACE)
target_link_libraries(cachemgr-utils INTERFACE cachemgr-utils-private)
target_include_directories(cachemgr-utils INTERFACE "${CMAKE_SOURCE_DIR}/src")

# don't add dependencies here, utils should be standalone and portable
