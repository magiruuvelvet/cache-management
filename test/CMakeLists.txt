# create executable
add_executable(cachemgr-tests
    utils_test/os_utils_test.cpp
    main_test.cpp
)

SetupTarget(cachemgr-tests "cachemgr-tests")

# include dependencies
target_link_libraries(cachemgr-tests PRIVATE cachemgr-utils)
target_link_libraries(cachemgr-tests PRIVATE libcachemgr)
target_link_libraries(cachemgr-tests PRIVATE Catch2) # Catch2 without default main()
