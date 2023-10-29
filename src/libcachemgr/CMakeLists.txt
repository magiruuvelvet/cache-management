# shared sources across all platforms
set(libcachemgr_shared_sources
    database/cache_db.cpp
    database/cache_db.hpp
    fs_watcher/fs_watcher.cpp
    fs_watcher/fs_watcher.hpp
    cachemgr.cpp
    cachemgr.hpp
    config_helper.hpp
    config.cpp
    config.hpp
    libcachemgr.cpp
    libcachemgr.hpp
    logging.cpp
    logging.hpp
    messages.hpp
    types.hpp
)

# include all package manager sources
file(GLOB_RECURSE libcachemgr_package_manager_sources
    "${CMAKE_CURRENT_SOURCE_DIR}/package_manager_support/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/package_manager_support/*.hpp"
)

# enable git version tracking if git is available
set(__libcachemgr_generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${__libcachemgr_generated_dir}/private")
EnableGitVersionTracking(
    "${CMAKE_CURRENT_SOURCE_DIR}/git_version.hpp.in"
    "${__libcachemgr_generated_dir}/private/git_version.hpp"
)

# for <mutex>
# 3rd party libraries already include this dependency,
# but require it explicitly here too since this library also uses threading
find_package(Threads REQUIRED)

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
    ${libcachemgr_package_manager_sources}
)

SetupTarget(libcachemgr-private "libcachemgr")

# ensure the git version file is generated before building libcachemgr
add_dependencies(libcachemgr-private generate_git_version_file)

# setup filesystem watcher backend compiler options
if (FILESYSTEM_WATCHER_BACKEND)
    # only expose this macro to this single source file where it is actually needed
    set_source_files_properties(fs_watcher/fs_watcher.cpp PROPERTIES
        COMPILE_DEFINITIONS FILESYSTEM_WATCHER_BACKEND="${FILESYSTEM_WATCHER_BACKEND}")
endif()

# include the generated git version information only in the file where it is needed
# avoids accidental use in other source files
set_source_files_properties(libcachemgr.cpp PROPERTIES
    INCLUDE_DIRECTORIES "${__libcachemgr_generated_dir}")

# include dependencies
target_link_libraries(libcachemgr-private PRIVATE Threads::Threads)
target_link_libraries(libcachemgr-private PRIVATE cachemgr-utils)
target_link_libraries(libcachemgr-private PRIVATE ryml) # rapidyaml
target_link_libraries(libcachemgr-private PRIVATE simdjson)
target_link_libraries(libcachemgr-private PRIVATE quill::quill)
target_link_libraries(libcachemgr-private PRIVATE sqlite3)

# create interface
add_library(libcachemgr INTERFACE)
target_link_libraries(libcachemgr INTERFACE libcachemgr-private)
target_include_directories(libcachemgr INTERFACE "${CMAKE_SOURCE_DIR}/src")
