#pragma once

#include <libcachemgr/package_manager_support/pm_base.hpp>

namespace libcachemgr {
namespace package_manager_support {

class npm : public pm_base
{
public:
    pm_name_type pm_name() const;

    /// using a configuration file, see {npmrc_cache_path}
    bool is_cache_directory_configurable() const;

    /**
     * note: `~/.npm` is allowed to be a symlink to another directory
     *
     * npm only blocks `node_modules` from being a symlink
     */
    bool is_cache_directory_symlink_compatible() const;

    std::string get_cache_directory_path() const;

private:
    /**
     * Parse the `npmrc` file and extract the `cache=` directory from it.
     *
     * Reference: https://docs.npmjs.com/cli/v10/configuring-npm/npmrc/
     *
     *  - per-project config file (/path/to/my/project/.npmrc)
     *  - per-user config file (~/.npmrc)
     *  - global config file ($PREFIX/etc/npmrc)
     *  - npm builtin config file (/path/to/npm/npmrc)
     */
    static std::string npmrc_cache_path();
};

} // namespace package_manager_support
} // namespace libcachemgr
