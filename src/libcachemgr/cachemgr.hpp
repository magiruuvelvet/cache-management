#pragma once

#include "types.hpp"
#include "config.hpp"
#include "package_manager_support/pm_base.hpp"

#include <string>
#include <list>
#include <initializer_list>
#include <system_error>

#include <utils/types/pointer.hpp>

/**
 * Cache Manager
 */
class cachemgr_t final
{
public:
    /**
     * Data structure to store comparison results for cache mappings.
     */
    struct cache_mappings_compare_results_t
    {
    public:
        /**
         * Standalone read-only copies of {libcachemgr::cache_mapping_t} for comparison.
         */
        struct cache_mappings_compare_result_t
        {
            /// the actual cache mapping, as found by the cache manager.
            const libcachemgr::configuration_t::cache_mapping_t actual;
            /// the expected cache mapping, as defined by the configuration file.
            const libcachemgr::configuration_t::cache_mapping_t expected;
        };

        using difference_t = std::list<const cache_mappings_compare_result_t>;
        using difference_size_type = difference_t::size_type;

        /**
         * Push a new comparison result to the internal list.
         *
         * @param result actual and expected {libcachemgr::cache_mapping_t} object
         */
        constexpr inline void add_result(const cache_mappings_compare_result_t &result)
        {
            this->_differences.emplace_back(result);
        }

        /**
         * Implicit boolean conversion operator to check if the comparison result has differences.
         *
         * @return true the internal result list contains differences
         * @return false the internal result list is empty
         */
        constexpr inline operator bool() const
        {
            // return true if the internal result list contains differences
            return this->_differences.size() > 0;
        }

        /**
         * Get the count of differences.
         */
        constexpr inline auto count() const
        {
            return this->_differences.size();
        }

        /**
         * Get the list of differences.
         */
        constexpr inline const auto &differences() const
        {
            return this->_differences;
        }

    private:
        /**
         * List of {cache_mappings_compare_result_t}.
         */
        difference_t _differences;
    };

    /**
     * Constructs and initializes a new cache manager.
     *
     * Once initialized, the {find_mapped_cache_directories} method should be called
     * to populate the state of the cache manager. This method can also be called to
     * update the state of the cache manager at any time.
     */
    cachemgr_t();

    /**
     * Destroys the cache manager and cleans up its resources.
     */
    virtual ~cachemgr_t();

    /**
     * Controls the sorting behavior of the {sorted_mapped_cache_directories} method.
     */
    enum class sort_behavior : unsigned
    {
        /// disable sorting (preserve order of elements as they are in the configuration file)
        unsorted = 0,
        /// elements with the least used disk space come first
        disk_usage_ascending = 1,
        /// elements with the most used disk space come first
        disk_usage_descending = 2,
    };

    /**
     * Finds all configured @p cache_mappings and validates if all of them exist on disk
     * and point to the expected target directories.
     *
     * Every cache mapping which differs from the expectations, will be added to the
     * comparison results for further inspection.
     *
     * @param cache_mappings the cache mapping list from the configuration file
     * @return comparison results (only contains differences)
     */
    cache_mappings_compare_results_t find_mapped_cache_directories(
        const libcachemgr::configuration_t::cache_mappings_t &cache_mappings) noexcept;

    /**
     * Returns the mapped cache directories.
     */
    inline constexpr const std::list<libcachemgr::mapped_cache_directory_t> &mapped_cache_directories() const {
        return this->_mapped_cache_directories;
    }

    /**
     * Get the count of mapped cache directories.
     */
    inline constexpr auto mapped_cache_directories_count() const {
        return this->_mapped_cache_directories.size();
    }

    /**
     * Receive a list of mapped cache directories, sorted by disk usage.
     *
     * Implementation notice:
     *   The returned list is a lightweight copy of const pointers to the original list.
     *   If {this} goes out of scope, all pointers in this list become dangling and accessing
     *   them results in undefined behavior.
     */
    const std::list<observer_ptr<libcachemgr::mapped_cache_directory_t>> sorted_mapped_cache_directories(
        sort_behavior sort_behavior = sort_behavior::disk_usage_descending) const noexcept;

    /**
     * Finds the corresponding cache mapping for the given @p pm_name (package manager).
     *
     * @param pm_name the name of the package manager
     * @return pointer to the corresponding cache mapping or nullptr if not found
     */
    const observer_ptr<libcachemgr::mapped_cache_directory_t> find_mapped_cache_directory_for_package_manager(
        libcachemgr::package_manager_support::pm_base::pm_name_type pm_name) const noexcept;

private:
    /**
     * List of mapped cache directories.
     */
    std::list<libcachemgr::mapped_cache_directory_t> _mapped_cache_directories;
};
