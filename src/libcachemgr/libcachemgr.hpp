#pragma once

#include <string>
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
    static const std::string_view application_name;

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
    static const std::string_view application_version;

    /**
     * the name of the platform on which the application is running
     *
     * NOTE: this value is obtained using cmake at configure time
     */
    static const std::string_view platform_name;

    // TODO: read git repository information at build time using cmake
    static const std::string_view git_branch;
    static const std::string_view git_commit;
    static const bool git_is_dirty;
};

/**
 * global state containing the user configuration obtained from the command line
 */
struct user_configuration_t final
{
public:
    static user_configuration_t *instance();

    void set_configuration_file(const std::string &configuration_file) noexcept;
    const std::string &configuration_file() const noexcept;

private:
    user_configuration_t() = default;

    std::string _configuration_file{};
};

inline user_configuration_t *user_configuration() noexcept
{
    return user_configuration_t::instance();
}

} // namespace libcachemgr
