# Detect the platform for which is being built and register the appropriate platform macros.
# Doing this at the build system stage is much easier than relying on compiler specific macros.
macro(DetectAndRegisterPlatformMacros Target)
    ####################################################################################################
    # Linux                                                                                            #
    ####################################################################################################

    if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
        set(PROJECT_PLATFORM_OS "Linux" CACHE STRING "" FORCE)
        target_compile_definitions(${Target} PRIVATE "-DPROJECT_PLATFORM_LINUX")

    ####################################################################################################
    # BSD family                                                                                       #
    ####################################################################################################

    elseif ("${CMAKE_SYSTEM_NAME}" STREQUAL "FreeBSD")
        set(PROJECT_PLATFORM_OS "FreeBSD" CACHE STRING "" FORCE)
        target_compile_definitions(${Target} PRIVATE "-DPROJECT_PLATFORM_FREEBSD")
        target_compile_definitions(${Target} PRIVATE "-DPROJECT_PLATFORM_BSD") # generic BSD catch-all

    elseif ("${CMAKE_SYSTEM_NAME}" STREQUAL "OpenBSD")
        set(PROJECT_PLATFORM_OS "OpenBSD" CACHE STRING "" FORCE)
        target_compile_definitions(${Target} PRIVATE "-DPROJECT_PLATFORM_OPENBSD")
        target_compile_definitions(${Target} PRIVATE "-DPROJECT_PLATFORM_BSD") # generic BSD catch-all

    elseif ("${CMAKE_SYSTEM_NAME}" STREQUAL "NetBSD")
        set(PROJECT_PLATFORM_OS "NetBSD" CACHE STRING "" FORCE)
        target_compile_definitions(${Target} PRIVATE "-DPROJECT_PLATFORM_NETBSD")
        target_compile_definitions(${Target} PRIVATE "-DPROJECT_PLATFORM_BSD") # generic BSD catch-all

    elseif ("${CMAKE_SYSTEM_NAME}" STREQUAL "DragonFlyBSD")
        set(PROJECT_PLATFORM_OS "DragonFlyBSD" CACHE STRING "" FORCE)
        target_compile_definitions(${Target} PRIVATE "-DPROJECT_PLATFORM_DRAGONFLYBSD")
        target_compile_definitions(${Target} PRIVATE "-DPROJECT_PLATFORM_BSD") # generic BSD catch-all

    ####################################################################################################
    # Darwin and macOS                                                                                 #
    ####################################################################################################

    elseif ("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
        set(PROJECT_PLATFORM_OS "Darwin" CACHE STRING "" FORCE)
        target_compile_definitions(${Target} PRIVATE "-DPROJECT_PLATFORM_DARWIN")
        if (APPLE)
            target_compile_definitions(${Target} PRIVATE "-DPROJECT_PLATFORM_MACOS")
        endif()

    ####################################################################################################
    # Windows                                                                                          #
    ####################################################################################################

    elseif ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
        set(PROJECT_PLATFORM_OS "Windows" CACHE STRING "" FORCE)
        target_compile_definitions(${Target} PRIVATE "-DPROJECT_PLATFORM_WINDOWS")
        target_compile_definitions(${Target} PRIVATE "-DPROJECT_PLATFORM_WINNT")

    ####################################################################################################
    # Unknown Platform                                                                                 #
    ####################################################################################################

    else()
        set(PROJECT_PLATFORM_OS "" CACHE STRING "" FORCE)
        target_compile_definitions(${Target} PRIVATE "-DPROJECT_PLATFORM_UNKNOWN")
        target_compile_definitions(${Target} PRIVATE "-DPROJECT_PLATFORM_UNDETECTED")
    endif()

    target_compile_definitions(${Target} PRIVATE "-DPROJECT_PLATFORM_NAME=\"${PROJECT_PLATFORM_OS}\"")
    message(STATUS "Detected platform: ${PROJECT_PLATFORM_OS}")
endmacro()
