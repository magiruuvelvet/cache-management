#pragma once

#include <libcachemgr/package_manager_support/pm_base.hpp>

namespace libcachemgr {
namespace package_manager_support {

class pub : public pm_base
{
public:
    constexpr pm_name_type pm_name() const {
        return "pub";
    }

    bool is_cache_directory_configurable() const;
    bool is_cache_directory_symlink_compatible() const;

    /**
     * pub package manager cache lookup:
     *
     *  - `$PUB_CACHE`
     *  - `$HOME/.pub-cache`
     */
    std::string get_cache_directory_path() const;
};

} // namespace package_manager_support
} // namespace libcachemgr
