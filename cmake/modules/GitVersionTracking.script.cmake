# disallow including this cmake module directly, as it is a script
if (
    "${GIT_VERSION_FILE_SOURCE}" STREQUAL "" OR
    "${GIT_VERSION_FILE_DESTINATION}" STREQUAL "" OR
    "${GIT_WORKING_DIRECTORY}" STREQUAL "" OR
    "${GIT_EXECUTABLE}" STREQUAL "")
    message(FATAL_ERROR "don't include() this cmake module directly")
endif()

macro(ExecuteGitCommand)
    execute_process(COMMAND
        "${GIT_EXECUTABLE}" ${ARGV}
        WORKING_DIRECTORY "${GIT_WORKING_DIRECTORY}"
        RESULT_VARIABLE exit_code
        OUTPUT_VARIABLE GIT_STDOUT
        ERROR_VARIABLE stderr
        OUTPUT_STRIP_TRAILING_WHITESPACE)

    if (exit_code EQUAL 0)
        set(GIT_RETRIEVED_STATE "true")
    else()
        set(GIT_RETRIEVED_STATE "false")
    endif()
endmacro()

# retrieve the current branch name
ExecuteGitCommand(symbolic-ref --short -q HEAD)
if (exit_code EQUAL 0)
    set(GIT_BRANCH "${GIT_STDOUT}")
else()
    set(GIT_BRANCH "")
endif()

# retrieve the current commit hash
ExecuteGitCommand(show -s "--format=%H" HEAD)
if (exit_code EQUAL 0)
    set(GIT_COMMIT "${GIT_STDOUT}")
else()
    set(GIT_COMMIT "")
endif()

# retrieve the repository clean/dirty state (excluding untracked files)
ExecuteGitCommand(status --porcelain -uno)
if (exit_code EQUAL 0)
    if("${GIT_STDOUT}" STREQUAL "")
        set(GIT_IS_DIRTY "false")
    else()
        set(GIT_IS_DIRTY "true")
    endif()
else()
    set(GIT_IS_DIRTY "false")
endif()

# retrieve the date of the current commit
ExecuteGitCommand(show -s "--format=%ci" HEAD)
if (exit_code EQUAL 0)
    # note: commit date will be in the committer's timezone
    set(GIT_COMMIT_DATE "${GIT_STDOUT}")
else()
    set(GIT_COMMIT_DATE "")
endif()

message(STATUS "GIT_RETRIEVED_STATE: ${GIT_RETRIEVED_STATE}")
message(STATUS "GIT_BRANCH:          ${GIT_BRANCH}")
message(STATUS "GIT_COMMIT:          ${GIT_COMMIT}")
message(STATUS "GIT_COMMIT_DATE:     ${GIT_COMMIT_DATE}")
message(STATUS "GIT_IS_DIRTY:        ${GIT_IS_DIRTY}")

# generate/update the git version file
configure_file("${GIT_VERSION_FILE_SOURCE}" "${GIT_VERSION_FILE_DESTINATION}" @ONLY)
