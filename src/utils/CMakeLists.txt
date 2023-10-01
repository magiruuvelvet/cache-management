# create library
add_library(cachemgr-utils-private STATIC
    logging_helper.cpp
    logging_helper.hpp
    os_utils.cpp
    os_utils.hpp
)

SetupTarget(cachemgr-utils-private "cachemgr-utils")

# create interface
add_library(cachemgr-utils INTERFACE)
target_link_libraries(cachemgr-utils INTERFACE cachemgr-utils-private)
target_include_directories(cachemgr-utils INTERFACE "${CMAKE_SOURCE_DIR}/src")
