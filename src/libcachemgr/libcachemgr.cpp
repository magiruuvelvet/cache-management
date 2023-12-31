#include "libcachemgr.hpp"

// only include this header file inside here and nowhere else
// this header file is automatically generated from git_version.hpp.in using cmake
#include <private/git_version.hpp>

#include <mutex>

#include <fmt/format.h>

using namespace libcachemgr;

const std::string_view program_metadata::application_name = "cachemgr";
const std::string_view program_metadata::application_version = "0.13.0";
const std::string_view program_metadata::application_version_suffix = "dev";
const std::string_view program_metadata::platform_name = PROJECT_PLATFORM_NAME;

// compiled-in git version information (will be blank when git is not available)
const bool program_metadata::git_retrieved_state{::git_retrieved_state};
const bool program_metadata::git_is_dirty{::git_is_dirty};
const std::string_view program_metadata::git_branch{::git_branch};
const std::string_view program_metadata::git_commit{::git_commit};
const std::string_view program_metadata::git_commit_date{::git_commit_date};

const std::string &program_metadata::full_application_version() noexcept
{
    static const std::string fav = ([]{
        // semver application version
        std::string buffer(program_metadata::application_version);

        if (program_metadata::application_version_suffix.size() > 0)
        {
            buffer.append(fmt::format("-{}", program_metadata::application_version_suffix));
        }

        // if git information is available, add it to the version string as semver build metadata
        // release builds should not have git information
        if constexpr (program_metadata::git_retrieved_state)
        {
            buffer.append(fmt::format("+{}-{}{}",
                program_metadata::git_branch,
                program_metadata::git_commit.substr(0, 10),
                program_metadata::git_is_dirty ? "-dirty" : ""));
        }

        // release builds don't have additional build metadata

        return buffer;
    })();

    return fav;
}

namespace {
    /// mutex for the {user_configuration_t} singleton instance
    static std::mutex user_configuration_mutex;
} // anonymous namespace

/// alias the type of the mutex lock mechanism for easier switching between them
using mutex_lock_t = std::lock_guard<std::mutex>;

user_configuration_t *user_configuration_t::instance() noexcept
{
    mutex_lock_t lock{user_configuration_mutex};
    static user_configuration_t instance;
    return &instance;
}

void user_configuration_t::set_configuration_file(const std::string &configuration_file) noexcept
{
    mutex_lock_t lock{user_configuration_mutex};
    this->_configuration_file = configuration_file;
}

const std::string &user_configuration_t::configuration_file() const noexcept
{
    mutex_lock_t lock{user_configuration_mutex};
    return this->_configuration_file;
}

void user_configuration_t::set_database_file(const std::string &database_file) noexcept
{
    mutex_lock_t lock{user_configuration_mutex};
    this->_database_file = database_file;
}

const std::string &user_configuration_t::database_file() const noexcept
{
    mutex_lock_t lock{user_configuration_mutex};
    return this->_database_file;
}

void user_configuration_t::set_verify_cache_mappings(bool verify_cache_mappings) noexcept
{
    mutex_lock_t lock{user_configuration_mutex};
    this->_verify_cache_mappings = verify_cache_mappings;
}

bool user_configuration_t::verify_cache_mappings() const noexcept
{
    mutex_lock_t lock{user_configuration_mutex};
    return this->_verify_cache_mappings;
}

void user_configuration_t::set_show_usage_stats(bool show_usage_stats) noexcept
{
    mutex_lock_t lock{user_configuration_mutex};
    this->_show_usage_stats = show_usage_stats;
}

bool user_configuration_t::show_usage_stats() const noexcept
{
    mutex_lock_t lock{user_configuration_mutex};
    return this->_show_usage_stats;
}

void user_configuration_t::set_print_pm_cache_locations(bool print_pm_cache_locations) noexcept
{
    mutex_lock_t lock{user_configuration_mutex};
    this->_print_pm_cache_locations = print_pm_cache_locations;
}

bool user_configuration_t::print_pm_cache_locations() const noexcept
{
    mutex_lock_t lock{user_configuration_mutex};
    return this->_print_pm_cache_locations;
}

void user_configuration_t::set_print_pm_cache_location_of(const std::string &package_manager) noexcept
{
    mutex_lock_t lock{user_configuration_mutex};
    this->_print_pm_cache_location_of = package_manager;
}

const std::string &user_configuration_t::print_pm_cache_location_of() const noexcept
{
    mutex_lock_t lock{user_configuration_mutex};
    return this->_print_pm_cache_location_of;
}
