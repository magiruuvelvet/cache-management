# find all source files
file(GLOB_RECURSE SourceList
    "*.cpp"
    "*.hpp"

    # TODO: migrate to library
    "../src/utils/*.cpp"
    "../src/utils/*.hpp"
)

# create executable
add_executable(cachemgr-tests ${SourceList})
set_target_properties(cachemgr-tests PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(cachemgr-tests PROPERTIES PREFIX "")
set_target_properties(cachemgr-tests PROPERTIES OUTPUT_NAME "cachemgr-tests")
DetectAndRegisterPlatformMacros(cachemgr-tests)

# force C++20 with extensions disabled
set_property(TARGET cachemgr-tests PROPERTY CXX_STANDARD 20)
set_property(TARGET cachemgr-tests PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET cachemgr-tests PROPERTY CXX_EXTENSIONS OFF)

# disable exceptions and runtime time information
target_compile_options(cachemgr-tests PRIVATE -fno-exceptions -fno-rtti)

# include dependencies
target_link_libraries(cachemgr-tests PRIVATE ryml) # rapidyaml
target_link_libraries(cachemgr-tests PRIVATE Catch2) # Catch2 without default main()

# add current directory to include paths
target_include_directories(cachemgr-tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(cachemgr-tests PRIVATE "${CMAKE_SOURCE_DIR}/src")
