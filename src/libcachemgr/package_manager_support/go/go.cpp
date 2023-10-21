#include "go.hpp"

#include <utils/os_utils.hpp>
#include <utils/freedesktop/xdg_paths.hpp>

using namespace libcachemgr::package_manager_support;

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
