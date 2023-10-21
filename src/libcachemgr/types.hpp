#pragma once

#include <string>
#include <list>

#include "package_manager_support/pm_base.hpp"

namespace libcachemgr {

/**
 * Defines a package manager for a {cache_mapping_t} object in a strict read-only manner.
 */
struct package_manager_t final
{
    /// require explicit construction
    inline constexpr explicit package_manager_t(const package_manager_support::pm_base *const pm) noexcept
        : pm(pm)
    {}

    /// evaluates to true if a package manager is present
    inline constexpr operator bool() const noexcept {
        return pm != nullptr;
    }

    /// access the package manager
    inline constexpr const auto operator()() const noexcept {
        return pm;
    }

private:
    const package_manager_support::pm_base *const pm{nullptr};
};

/**
 * Type of the cache directory.
 */
enum class directory_type_t : unsigned
{
    /// symbolic link to a directory
    symbolic_link = 0,
    /// bind mount
    bind_mount = 1,
    /// standalone cache target without source directory
    standalone = 2,
    /// wildcard match without target directory
    wildcard = 3,

    /// invalid value
    invalid = 255,
};

/**
 * Structure containing information about a cache directory mapping.
 */
struct mapped_cache_directory_t
{
    /**
     * The type of the cache directory {original_path}.
     */
    const libcachemgr::directory_type_t directory_type;

    /**
     * The original path, which should be a symbolic link or bind mount.
     */
    const std::string original_path;

    /**
     * The target directory of the symbolic link or bind mount.
     */
    const std::string target_path;

    /**
     * Read-only pointer to the corresponding package manager.
     */
    const libcachemgr::package_manager_t package_manager;

    /**
     * List of resolved source files when wildcard matching is used.
     */
    const std::list<std::string> resolved_source_files;

    /**
     * The original wildcard pattern which was used to build the list of resolved source files.
     */
    const std::string wildcard_pattern;

    /**
     * The size on disk of the target directory {target_path}.
     * This property can be mutated in const contexts.
     */
    mutable std::uintmax_t disk_size{0};

    // Implementation details:
    //  - use inclusive matching only for directory types

    /**
     * Checks if the mapping has a target directory.
     */
    inline constexpr bool has_target_directory() const {
        return (
            this->directory_type == libcachemgr::directory_type_t::symbolic_link ||
            this->directory_type == libcachemgr::directory_type_t::bind_mount ||
            this->directory_type == libcachemgr::directory_type_t::standalone
        ) && this->target_path.size() > 0;
    }

    /**
     * Checks if the mapping has wildcard matches.
     */
    inline constexpr bool has_wildcard_matches() const {
        return this->directory_type == libcachemgr::directory_type_t::wildcard &&
            this->resolved_source_files.size() > 0;
    }
};

} // namespace libcachemgr
