#pragma once

#include <string>
#include <string_view>

namespace libcachemgr {
namespace package_manager_support {

/**
 * Abstract base class to implement a package manager integration.
 */
class pm_base
{
public:
    constexpr pm_base() = default;
    virtual ~pm_base() = default;

    /// alias the type of the package manager name (just to be sure)
    using pm_name_type = std::string_view;

    /**
     * The name of the package manager.
     */
    virtual constexpr pm_name_type pm_name() const = 0;

    /**
     * This method should return whether the package manager allows its
     * cache directory to be changed by the user.
     *
     * Be it with a configuration file, an environment variable, or both.
     *
     * @return true cache directory can be changed by the user
     * @return false cache directory cannot be changed by the user
     */
    virtual bool is_cache_directory_configurable() const = 0;

    /**
     * This method should return whether the package manager allows its
     * cache directory to be a symbolic link.
     *
     * This method should only return false if the package manager enforces
     * its cache directory to be a regular directory.
     *
     * A possible workaround for such a package manager is to use a bind mount.
     *
     * @return true cache directory can be a symbolic link
     * @return false cache directory cannot be a symbolic link
     */
    virtual bool is_cache_directory_symlink_compatible() const = 0;

    /**
     * This method should return the path to the package manager's cache directory
     * which is currently in use.
     *
     * Some package managers might return different paths depending on the current
     * context (like the process working directory for example).
     *
     * This method should parse a configuration file or fetch an environment variable
     * according to the specifications of the package manager.
     *
     * If the package manager doesn't allow its cache directory to be changed by the user,
     * this method should return the hardcoded default cache directory of that package manager.
     *
     * @return cache directory which is currently in use
     */
    virtual std::string get_cache_directory_path() const = 0;
};

} // namespace package_manager_support
} // namespace libcachemgr
