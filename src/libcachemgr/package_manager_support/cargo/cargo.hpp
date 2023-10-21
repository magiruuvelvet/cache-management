#pragma once

#include <libcachemgr/package_manager_support/pm_base.hpp>

namespace libcachemgr {
namespace package_manager_support {

class cargo : public pm_base
{
public:
    constexpr pm_name_type pm_name() const {
        return "cargo";
    }

    bool is_cache_directory_configurable() const;
    bool is_cache_directory_symlink_compatible() const;

    /**
     * cargo cache lookup:
     *
     * `$CARGO_HOME` contains more than just caches.
     *
     * References:
     *  - https://doc.rust-lang.org/cargo/guide/cargo-home.html
     *  - https://github.com/rust-lang/cargo/issues/10252
     *  - https://github.com/rust-lang/cargo/issues/1734
     *  - https://poignardazur.github.io/2023/05/23/platform-compliance-in-cargo/
     *
     * locations which are definitely cache and safe to delete for any reason:
     *  - `$CARGO_HOME/registry` - local crates registry and source code
     *  - `$CARGO_HOME/git`      - cloned git repositories
     *
     * `$CARGO_HOME` defaults to `$HOME/.cargo` if not set.
     *
     * TODO: properly support cargo, for now just treat the entire `$CARGO_HOME` as cache.
     */
    std::string get_cache_directory_path() const;
};

} // namespace package_manager_support
} // namespace libcachemgr
