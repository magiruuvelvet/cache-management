#include "go.hpp"

#include <utils/os_utils.hpp>
#include <utils/freedesktop/xdg_paths.hpp>

using namespace libcachemgr::package_manager_support;

pm_base::pm_name_type go::pm_name() const
{
    return "go";
}

bool go::is_cache_directory_configurable() const
{
    return true;
}

bool go::is_cache_directory_symlink_compatible() const
{
    return true;
}

std::string go::get_cache_directory_path() const
{
    return os_utils::getenv("GOCACHE", []{
        return freedesktop::xdg_paths::get_xdg_cache_home() + "/go-build";
    });
}
