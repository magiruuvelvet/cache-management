#pragma once

#include <libcachemgr/package_manager_support/pm_base.hpp>

namespace libcachemgr {
namespace package_manager_support {

class cargo : public pm_base
{
public:
    pm_name_type pm_name() const;
    bool is_cache_directory_configurable() const;
    bool is_cache_directory_symlink_compatible() const;

    /**
     * cargo cache lookup:
     *
     * `$CARGO_HOME` contains more than just caches.
     * Reference: https://doc.rust-lang.org/cargo/guide/cargo-home.html
     *
     * `$CARGO_HOME` defaults to `$HOME/.cargo` if not set.
     *
     * TODO: properly support cargo, for now just treat the entire `$CARGO_HOME` as cache.
     */
    std::string get_cache_directory_path() const;
};

} // namespace package_manager_support
} // namespace libcachemgr
