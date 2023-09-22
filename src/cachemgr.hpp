#pragma once

#include <string>
#include <list>
#include <system_error>

/**
 * Cache Manager
 */
class cachemgr_t final
{
public:
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
     * Structure containing information about a cache directory mapping.
     */
    struct symlinked_cache_directory_t
    {
        /**
         * The original path, which should be a symbolic link.
         */
        const std::string original_path;

        /**
         * The target directory of the symbolic link.
         */
        const std::string symlinked_path;
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
     * Returns the symlinked cache directories.
     */
    inline constexpr const std::list<symlinked_cache_directory_t> &symlinked_cache_directories() const
    {
        return this->_symlinked_cache_directories;
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
    std::list<symlinked_cache_directory_t> _symlinked_cache_directories;

    /**
     * Last system error encountered by the cache manager.
     */
    std::error_code _last_system_error;
};
