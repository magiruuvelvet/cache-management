#pragma once

#include <libcachemgr/package_manager_support/pm_base.hpp>

namespace libcachemgr {
namespace package_manager_support {

class go : public pm_base
{
public:
    constexpr pm_name_type pm_name() const {
        return "go";
    }

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
