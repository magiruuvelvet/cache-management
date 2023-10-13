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
 * Represents a cache mapping in the configuration file.
 */
struct cache_mapping_t final
{
    const std::string type; // unused for now, type might change in the future
    const package_manager_t package_manager;
    const std::string source;
    const std::string target;
};

/**
 * List of cache mappings in the configuration file.
 */
using cache_mappings_t = std::list<cache_mapping_t>;

} // namespace libcachemgr
