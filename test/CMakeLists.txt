# create executable
add_executable(cachemgr-tests
    include/test_helper.hpp
    libcachemgr_test/config_test.cpp
    utils_test/freedesktop_test/os-release_test.cpp
    utils_test/freedesktop_test/xdg_paths_test.cpp
    utils_test/os_utils_test.cpp
    main_test.cpp
)

SetupTarget(cachemgr-tests "cachemgr-tests")

# include dependencies
target_link_libraries(cachemgr-tests PRIVATE cachemgr-utils)
target_link_libraries(cachemgr-tests PRIVATE libcachemgr)
target_link_libraries(cachemgr-tests PRIVATE Catch2) # Catch2 without default main()
target_link_libraries(cachemgr-tests PRIVATE quill::quill)

target_include_directories(cachemgr-tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

# get absolute directory to test assets
set(CACHEMGR_TESTS_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
message(STATUS "Test assets directory: ${CACHEMGR_TESTS_ASSETS_DIR}")
target_compile_definitions(cachemgr-tests PRIVATE "-DCACHEMGR_TESTS_ASSETS_DIR=\"${CACHEMGR_TESTS_ASSETS_DIR}\"")
