macro(SetupTarget Target OutputName)
    set_target_properties(${Target} PROPERTIES LINKER_LANGUAGE CXX)
    set_target_properties(${Target} PROPERTIES PREFIX "")
    set_target_properties(${Target} PROPERTIES OUTPUT_NAME "${OutputName}")
    DetectAndRegisterPlatformMacros(${Target})

    # force C++20 with extensions disabled
    set_property(TARGET ${Target} PROPERTY CXX_STANDARD 20)
    set_property(TARGET ${Target} PROPERTY CXX_STANDARD_REQUIRED ON)
    set_property(TARGET ${Target} PROPERTY CXX_EXTENSIONS OFF)

    # disable exceptions and runtime time information
    target_compile_options(${Target} PRIVATE -fno-exceptions -fno-rtti)

    # add current directory to include paths
    target_include_directories(${Target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
endmacro()
