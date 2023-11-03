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
     * useful tool: https://jubianchi.github.io/semver-check
     */
    static const std::string_view application_version;
    // examples: dev, alpha, beta, rc.1, rc.2, rc.3, rc.4, rc.5
    static const std::string_view application_version_suffix;

    /// retrieve the full semver compliant application version string (including git version information)
    static const std::string &full_application_version() noexcept;

    /**
     * the name of the platform on which the application is running
     *
     * NOTE: this value is obtained using cmake at configure time
     */
    static const std::string_view platform_name;

    // compiled-in git version information (will be blank when git is not available)

    /// whether git version information has been retrieved or not.
    /// this is also false when git was disabled at configure time.
    static const bool git_retrieved_state;

    /// this flag is true when the git repository is in dirty state (excluding untracked files)
    static const bool git_is_dirty;

    /// the currently checked out git branch
    static const std::string_view git_branch;

    /// the currently checked out git commit
    static const std::string_view git_commit;

    /// the date of the currently checked out git commit
    static const std::string_view git_commit_date;
};

/**
 * global state containing the user configuration obtained from the command line
 *
 * NOTE: thread-safe singleton
 */
struct user_configuration_t final
{
public:
    /// get the singleton instance (instance will be constructed upon first call)
    static user_configuration_t *instance() noexcept;

    void set_configuration_file(const std::string &configuration_file) noexcept;
    const std::string &configuration_file() const noexcept;

    void set_database_file(const std::string &database_file) noexcept;
    const std::string &database_file() const noexcept;

    void set_verify_cache_mappings(bool verify_cache_mappings) noexcept;
    bool verify_cache_mappings() const noexcept;

    void set_show_usage_stats(bool show_usage_stats) noexcept;
    bool show_usage_stats() const noexcept;

    void set_print_pm_cache_locations(bool print_pm_cache_locations) noexcept;
    bool print_pm_cache_locations() const noexcept;

    void set_print_pm_cache_location_of(const std::string &package_manager) noexcept;
    const std::string &print_pm_cache_location_of() const noexcept;

private:
    user_configuration_t() = default;

    std::string _configuration_file{};
    std::string _database_file{};
    std::string _print_pm_cache_location_of{};
    bool _verify_cache_mappings{false};
    bool _show_usage_stats{false};
    bool _print_pm_cache_locations{false};
};

/**
 * convenience function to get the user configuration singleton instance
 */
[[nodiscard]] inline user_configuration_t *user_configuration() noexcept
{
    return user_configuration_t::instance();
}

} // namespace libcachemgr
