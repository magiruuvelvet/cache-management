#include "libcachemgr.hpp"

#include <mutex>

using namespace libcachemgr;

const std::string_view program_metadata::application_name = "cachemgr";
const std::string_view program_metadata::application_version = "0.0.0-dev";
const std::string_view program_metadata::platform_name = PROJECT_PLATFORM_NAME;

const std::string_view program_metadata::git_branch{};
const std::string_view program_metadata::git_commit{};
const bool program_metadata::git_is_dirty{};

namespace {
    /// mutex for the {user_configuration_t} singleton instance
    static std::mutex user_configuration_mutex;
} // anonymous namespace

user_configuration_t *user_configuration_t::instance()
{
    std::lock_guard{user_configuration_mutex};
    static user_configuration_t instance;
    return &instance;
}

void user_configuration_t::set_configuration_file(const std::string &configuration_file) noexcept
{
    std::lock_guard{user_configuration_mutex};
    this->_configuration_file = configuration_file;
}

const std::string &user_configuration_t::configuration_file() const noexcept
{
    std::lock_guard{user_configuration_mutex};
    return this->_configuration_file;
}
