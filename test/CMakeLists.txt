# get absolute directory to test assets
set(CACHEMGR_TESTS_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
message(STATUS "Test assets directory: ${CACHEMGR_TESTS_ASSETS_DIR}")

add_executable(cachemgr-tests
    include/test_helper.hpp
    libcachemgr_test/config_test.cpp
    package_manager_support_test/composer_test.cpp
    package_manager_support_test/go_test.cpp
    package_manager_support_test/npm_test.cpp
    package_manager_support_test/pub_test.cpp
    utils_test/freedesktop_test/os-release_test.cpp
    utils_test/freedesktop_test/xdg_paths_test.cpp
    utils_test/os_utils_test.cpp
    main_test.cpp
)

add_executable(cachemgr-test-pm-composer
    include/test_helper.hpp
    package_manager_support_test/composer_test_cache_dir.cpp
    main_test.cpp
)

add_executable(cachemgr-test-database
    include/test_helper.hpp
    database_test.cpp
)

macro(SetupTestTarget TestTarget)
    SetupTarget(${TestTarget} "${TestTarget}")

    # include dependencies
    target_link_libraries(${TestTarget} PRIVATE cachemgr-utils)
    target_link_libraries(${TestTarget} PRIVATE libcachemgr)
    target_link_libraries(${TestTarget} PRIVATE Catch2) # Catch2 without default main()
    target_link_libraries(${TestTarget} PRIVATE quill::quill)

    target_include_directories(${TestTarget} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

    target_compile_definitions(${TestTarget} PRIVATE "-DCACHEMGR_TESTS_ASSETS_DIR=\"${CACHEMGR_TESTS_ASSETS_DIR}\"")
endmacro()

# create a unique test binary for each test case which needs to modify the process state,
# including but not limited to:
#   - changing the process working directory
#
# modifying the process state is considered bad practice for unit testing and can cause weird data races

SetupTestTarget(cachemgr-tests)
SetupTestTarget(cachemgr-test-pm-composer)
SetupTestTarget(cachemgr-test-database)
