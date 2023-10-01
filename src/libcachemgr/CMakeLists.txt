# create library
add_library(libcachemgr-private STATIC
    cachemgr.cpp
    cachemgr.hpp
    config.cpp
    config.hpp
    logging.cpp
    logging.hpp
)

SetupTarget(libcachemgr-private "libcachemgr")

# include dependencies
target_link_libraries(libcachemgr-private PRIVATE cachemgr-utils)
target_link_libraries(libcachemgr-private PRIVATE ryml) # rapidyaml
target_link_libraries(libcachemgr-private PRIVATE quill::quill)

# create interface
add_library(libcachemgr INTERFACE)
target_link_libraries(libcachemgr INTERFACE libcachemgr-private)
target_include_directories(libcachemgr INTERFACE "${CMAKE_SOURCE_DIR}/src")
