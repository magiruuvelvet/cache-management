#include "cargo.hpp"

#include <utils/os_utils.hpp>

using namespace libcachemgr::package_manager_support;

pm_base::pm_name_type cargo::pm_name() const
{
    return "cargo";
}

bool cargo::is_cache_directory_configurable() const
{
    // TODO: should this really return true? since there is no unified cache directory for cargo
    return true;
}

bool cargo::is_cache_directory_symlink_compatible() const
{
    // `$CARGO_HOME` can be a symlink
    return true;
}

std::string cargo::get_cache_directory_path() const
{
    return os_utils::getenv("CARGO_HOME", []{
        return os_utils::get_home_directory() + "/.cargo";
    });
}
