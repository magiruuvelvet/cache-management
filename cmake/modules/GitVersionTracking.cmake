if (GIT_FOUND)
    macro(EnableGitVersionTracking GitVersionFileSource GitVersionFileDestination)
        message(STATUS "Enabling git version tracking")
        add_custom_target(generate_git_version_file
            ALL
            DEPENDS ${GitVersionFileSource}
            BYPRODUCTS
                ${GitVersionFileDestination}
            COMMENT "Updating the git version file..."
            COMMAND
                ${CMAKE_COMMAND}
                -DGIT_WORKING_DIRECTORY="${CMAKE_SOURCE_DIR}"
                -DGIT_EXECUTABLE="${GIT_EXECUTABLE}"
                -DGIT_VERSION_FILE_SOURCE="${GitVersionFileSource}"
                -DGIT_VERSION_FILE_DESTINATION="${GitVersionFileDestination}"
                -P "${CMAKE_SOURCE_DIR}/cmake/modules/GitVersionTracking.script.cmake")
    endmacro()
else()
    macro(EnableGitVersionTracking GitVersionFileSource GitVersionFileDestination)
        # generate blank git version file
        set(GIT_RETRIEVED_STATE "false")
        set(GIT_IS_DIRTY "false")
        configure_file("${GitVersionFileSource}" "${GitVersionFileDestination}" @ONLY)
        add_custom_target(generate_git_version_file)
    endmacro()
endif()
