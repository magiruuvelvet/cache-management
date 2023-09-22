# find all source files
file(GLOB_RECURSE SourceList
    "*.cpp"
    "*.hpp"
)

# create executable
add_executable(cachemgr ${SourceList})
set_target_properties(cachemgr PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(cachemgr PROPERTIES PREFIX "")
set_target_properties(cachemgr PROPERTIES OUTPUT_NAME "cachemgr")
DetectAndRegisterPlatformMacros(cachemgr)

# force C++20 with extensions disabled
set_property(TARGET cachemgr PROPERTY CXX_STANDARD 20)
set_property(TARGET cachemgr PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET cachemgr PROPERTY CXX_EXTENSIONS OFF)

# disable exceptions and runtime time information
target_compile_options(cachemgr PRIVATE -fno-exceptions -fno-rtti)
