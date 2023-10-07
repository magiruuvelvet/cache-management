#pragma once

#include "cache_mapping.hpp"

#include <string>
#include <list>
#include <initializer_list>
#include <system_error>

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
            const libcachemgr::cache_mapping_t actual;
            /// the expected cache mapping, as defined by the configuration file.
            const libcachemgr::cache_mapping_t expected;
        };

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
        std::list<const cache_mappings_compare_result_t> _differences;
    };

    /**
     * Constructs and initializes a new cache manager.
     *
     * Once initialized, the {find_symlinked_cache_directories} method should be called
     * to populate the state of the cache manager. This method can also be called to
     * update the state of the cache manager at any time.
     */
    cachemgr_t();

    /**
     * Destroys the cache manager and cleans up its resources.
     */
    virtual ~cachemgr_t();

    /**
     * Type of the cache directory.
     */
    enum class directory_type_t : unsigned {
        /// symbolic link to a directory
        symbolic_link = 0,
        /// bind mount
        bind_mount = 1,
    };

    /**
     * Structure containing information about a cache directory mapping.
     */
    struct mapped_cache_directory_t
    {
        /**
         * The type of the cache directory {original_path}.
         */
        const directory_type_t directory_type;

        /**
         * The original path, which should be a symbolic link or bind mount.
         */
        const std::string original_path;

        /**
         * The target directory of the symbolic link or bind mount.
         */
        const std::string target_path;

        /**
         * The size on disk of the target directory {target_path}.
         * This property can be mutated in const contexts.
         */
        mutable std::uintmax_t disk_size{0};
    };

    /**
     * Searches the given directory for any symlinked cache directories.
     *
     * Only symbolic links which point to directories starting with the {symlink_target_prefix} are used.
     *
     * The results are stored in {_symlinked_cache_directories}.
     *
     * @param path directory in which symlinked cache directories should be searched
     * @param symlink_target_prefix the prefix of the symlink target, empty to include all symlink targets
     * @return true search was successful
     * @return false error accessing directory, see {get_last_system_error} for an explanation
     */
    bool find_symlinked_cache_directories(const std::string &path, const std::string &symlink_target_prefix = {}) noexcept;

    /**
     * Searches the given directories for any symlinked cache directories.
     *
     * Only symbolic links which point to directories starting with the {symlink_target_prefix} are used.
     *
     * The results are stored in {_symlinked_cache_directories}.
     *
     * @param paths directories in which symlinked cache directories should be searched
     * @param symlink_target_prefix the prefix of the symlink target, empty to include all symlink targets
     * @return true search was successful
     * @return false error accessing directory, see {get_last_system_error} for an explanation
     */
    bool find_symlinked_cache_directories(
        const std::initializer_list<std::string> &paths, const std::string &symlink_target_prefix = {}) noexcept;

    /**
     * Compares the given cache mapping list (from the configuration file) with the found symlinked cache directories.
     *
     * The results are stored in comparison results structure.
     *
     * @param cache_mappings the cache mapping list from the configuration file
     * @return comparison results
     */
    cache_mappings_compare_results_t compare_cache_mappings(const libcachemgr::cache_mappings_t &cache_mappings) const noexcept;

    /**
     * Returns the symlinked cache directories.
     */
    inline constexpr const std::list<mapped_cache_directory_t> &symlinked_cache_directories() const
    {
        return this->_mapped_cache_directories;
    }

    /**
     * Returns the last system error encountered by the cache manager.
     */
    inline constexpr const std::error_code &get_last_system_error() const
    {
        return this->_last_system_error;
    }

private:
    /**
     * List of symlinked cache directories.
     */
    std::list<mapped_cache_directory_t> _mapped_cache_directories;

    /**
     * Last system error encountered by the cache manager.
     */
    std::error_code _last_system_error;

    /**
     * Helper boolean to determine if the previous {_symlinked_cache_directories} should be cleared
     * before rescanning the given directories.
     */
    bool _clear_previous_list = true;
};
