#pragma once

#include <libcachemgr/package_manager_support/pm_base.hpp>

namespace libcachemgr {
namespace package_manager_support {

class go : public pm_base
{
public:
    pm_name_type pm_name() const;
    bool is_cache_directory_configurable() const;
    bool is_cache_directory_symlink_compatible() const;

    /**
     * Go compiler cache lookup:
     *
     *  - `$GOCACHE`
     *  - `$XDG_CACHE_HOME/go-build`
     */
    std::string get_cache_directory_path() const;
};

} // namespace package_manager_support
} // namespace libcachemgr
