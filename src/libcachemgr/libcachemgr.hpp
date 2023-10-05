#pragma once

#include <string_view>

namespace libcachemgr {

/**
 * centralized version information and metadata about the program
 */
struct program_metadata final
{
    /**
     * application name
     */
    static std::string_view application_name;

    /**
     * application version number
     *
     * adhere to semantic versioning 2.0.0, see https://semver.org/
     *
     * Q: When should I increment the major version number?
     * A: When there are user-facing breaking changes to the command line interface
     *    or the behavior of the program. `libcachemgr` is a private library and is
     *    not intended for public consumption, so it doesn't need to adhere to semver.
     *
     * TODO: add `-dirty` suffix to indicate that this is a dirty build
     */
    static std::string_view application_version;

    /**
     * the name of the platform on which the application is running
     *
     * NOTE: this value is obtained using cmake at configure time
     */
    static std::string_view platform_name;

    // TODO: read git repository information at build time using cmake
    static std::string_view git_branch;
    static std::string_view git_commit;
    static bool git_is_dirty;
};

} // namespace libcachemgr
