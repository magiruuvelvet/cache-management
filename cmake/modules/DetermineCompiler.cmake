# Determine the compiler vendor to control compiler options.
macro(DetermineCompiler)
    # LLVM clang
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(PROJECT_COMPILER_CLANG TRUE CACHE BOOL "" FORCE)

    # GNU GCC
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(PROJECT_COMPILER_GCC TRUE CACHE BOOL "" FORCE)
    endif()
endmacro()
