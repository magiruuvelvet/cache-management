#pragma once

#include <libcachemgr/package_manager_support/pm_base.hpp>

namespace libcachemgr {
namespace package_manager_support {

class composer : public pm_base
{
public:
    pm_name_type pm_name() const;
    bool is_cache_directory_configurable() const;
    bool is_cache_directory_symlink_compatible() const;

    /**
     * composer cache lookup:
     *
     *  - project config file: `$PWD/composer.json` -> key: `config.cache-dir`
     *  - global config file: `$XDG_CONFIG_HOME/composer/config.json` -> key: `config.cache-dir`
     *  - `$XDG_CACHE_HOME/composer`
     *  - `$COMPOSER_HOME/cache`
     *
     * `$COMPOSER_HOME` seems to default to `$XDG_CONFIG_HOME/composer` when not set.
     *
     * TODO: Need to check composer source code for correct behavior.
     * TODO: Respect `$COMPOSER_CACHE_DIR`
     *
     * References:
     *  - https://getcomposer.org/doc/06-config.md#cache-dir
     *  - https://getcomposer.org/doc/03-cli.md#composer-home
     *  - https://getcomposer.org/doc/03-cli.md#composer-cache-dir
     *  - `composer config --global --help`
     *  - `composer config --global --list`
     *  - `composer config --global cache-dir`
     *  - `composer config cache-dir` in local test project with `composer.json`
     */
    std::string get_cache_directory_path() const;

private:
    /**
     * Helper function around `$COMPOSER_HOME` and `$XDG_CONFIG_HOME/composer`.
     */
    std::string get_composer_home_path() const;
};

} // namespace package_manager_support
} // namespace libcachemgr
