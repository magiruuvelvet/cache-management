#include "pub.hpp"

#include <utils/os_utils.hpp>

using namespace libcachemgr::package_manager_support;

bool pub::is_cache_directory_configurable() const
{
    return true;
}

bool pub::is_cache_directory_symlink_compatible() const
{
    return true;
}

std::string pub::get_cache_directory_path() const
{
    return os_utils::getenv("PUB_CACHE", []{
        return os_utils::get_home_directory() + "/.pub-cache";
    });
}
