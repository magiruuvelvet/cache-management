#include "libcachemgr.hpp"

// only include this header file inside here and nowhere else
// this header file is automatically generated from git_version.hpp.in using cmake
#include <private/git_version.hpp>

#include <mutex>

using namespace libcachemgr;

const std::string_view program_metadata::application_name = "cachemgr";
const std::string_view program_metadata::application_version = "0.0.0-dev";
const std::string_view program_metadata::platform_name = PROJECT_PLATFORM_NAME;

// compiled-in git version information (will be blank when git is not available)
const bool program_metadata::git_retrieved_state{::git_retrieved_state};
const bool program_metadata::git_is_dirty{::git_is_dirty};
const std::string_view program_metadata::git_branch{::git_branch};
const std::string_view program_metadata::git_commit{::git_commit};
const std::string_view program_metadata::git_commit_date{::git_commit_date};

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
