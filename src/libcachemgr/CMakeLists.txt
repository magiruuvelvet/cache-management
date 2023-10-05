# shared sources across all platforms
set(libcachemgr_shared_sources
    fs_watcher/fs_watcher.cpp
    fs_watcher/fs_watcher.hpp
    cache_mapping.hpp
    cachemgr.cpp
    cachemgr.hpp
    config.cpp
    config.hpp
    libcachemgr.cpp
    libcachemgr.hpp
    logging.cpp
    logging.hpp
)

# determine the backend for the filesystem watcher
if (PROJECT_PLATFORM_LINUX)
    set(FILESYSTEM_WATCHER_BACKEND "inotify" CACHE STRING "" FORCE)
endif()
# NOTE: add more filesystem watcher backends here

# log the found filesystem watcher backend
if (FILESYSTEM_WATCHER_BACKEND)
    message(STATUS "filesystem watcher backend: ${FILESYSTEM_WATCHER_BACKEND}")
endif()

# setup filesystem watcher backend source files
if ("${FILESYSTEM_WATCHER_BACKEND}" STREQUAL "inotify")
    set(libcachemgr_fs_watcher_backend_sources
        fs_watcher/backends/inotify.cpp
        fs_watcher/backends/inotify.hpp
    )
else()
    message(WARNING "no backend for the filesystem watcher found, realtime monitoring of files will not be available")
    set(libcachemgr_fs_watcher_backend_sources)
endif()

# create library
add_library(libcachemgr-private STATIC
    ${libcachemgr_shared_sources}
    ${libcachemgr_fs_watcher_backend_sources}
)

SetupTarget(libcachemgr-private "libcachemgr")

# setup filesystem watcher backend compiler options
if (FILESYSTEM_WATCHER_BACKEND)
    # only expose this macro to this single source file where it is actually needed
    set_source_files_properties(fs_watcher/fs_watcher.cpp PROPERTIES
        COMPILE_DEFINITIONS FILESYSTEM_WATCHER_BACKEND="${FILESYSTEM_WATCHER_BACKEND}")
endif()

# include dependencies
target_link_libraries(libcachemgr-private PRIVATE cachemgr-utils)
target_link_libraries(libcachemgr-private PRIVATE ryml) # rapidyaml
target_link_libraries(libcachemgr-private PRIVATE quill::quill)

# create interface
add_library(libcachemgr INTERFACE)
target_link_libraries(libcachemgr INTERFACE libcachemgr-private)
target_include_directories(libcachemgr INTERFACE "${CMAKE_SOURCE_DIR}/src")
