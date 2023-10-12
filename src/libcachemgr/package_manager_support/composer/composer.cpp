#include "composer.hpp"

#include <utils/os_utils.hpp>
#include <utils/freedesktop/xdg_paths.hpp>

#include <libcachemgr/logging.hpp>

using namespace libcachemgr::package_manager_support;

namespace {

static constexpr const char *LOG_TAG = "composer";
static constexpr const char *COMPOSER_HOME = "COMPOSER_HOME";

} // anonymous namespace

pm_base::pm_name_type composer::pm_name() const
{
    return "composer";
}

bool composer::is_cache_directory_configurable() const
{
    return true;
}

bool composer::is_cache_directory_symlink_compatible() const
{
    return true;
}

std::string composer::get_cache_directory_path() const
{
    // TODO: read json configuration file (need an exception-free json parser with prober error handling instead of calling std::abort)

    bool exists = false;
    if (const auto composer_home = os_utils::getenv(COMPOSER_HOME, &exists); exists)
    {
        LOG_INFO(libcachemgr::log_pm, "[{}] using {}: {}", LOG_TAG, COMPOSER_HOME, composer_home);
        return composer_home + "/cache";
    }
    else
    {
        LOG_INFO(libcachemgr::log_pm, "[{}] using xdg cache home", LOG_TAG);
        return freedesktop::xdg_paths::get_xdg_cache_home() + "/composer";
    }
}

std::string composer::get_composer_home_path() const
{
    return os_utils::getenv(COMPOSER_HOME, []{
        return freedesktop::xdg_paths::get_xdg_config_home() + "/composer";
    });
}
