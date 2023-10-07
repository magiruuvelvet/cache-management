#include "cachemgr.hpp"
#include "logging.hpp"

#include <filesystem>
#include <unordered_map>
#include <string_view>

#include <utils/os_utils.hpp>

namespace fs = std::filesystem;

namespace {
    /// callback to execute after the return statement
    struct after_run_callback final
    {
        after_run_callback(bool *clear_previous_list)
            : clear_previous_list(clear_previous_list) {}
        ~after_run_callback()
        {
            // reset back to default value
            *this->clear_previous_list = true;
        }

        bool *clear_previous_list = nullptr;
    };
} // anonymous namespace

cachemgr_t::cachemgr_t()
{
}

cachemgr_t::~cachemgr_t()
{
}

bool cachemgr_t::find_symlinked_cache_directories(const std::string &path, const std::string &symlink_target_prefix) noexcept
{
    // ensure the {_clear_previous_list} boolean is always set to true after this function returns
    after_run_callback arc(&this->_clear_previous_list);

    // clear previous results
    if (this->_clear_previous_list)
    {
        this->_mapped_cache_directories.clear();
    }

    // input path must be a directory
    if (!fs::is_directory(path, this->_last_system_error))
    {
        return false;
    }

    // iterate over the given directory and search for symlinked directories
    for (const auto &entry : fs::directory_iterator(path))
    {
        if (entry.is_symlink())
        {
            // resolve the symbolic link
            const std::string symlink_target = fs::read_symlink(entry.path(), this->_last_system_error);

            // abort on errors immediately
            if (this->_last_system_error)
            {
                LOG_ERROR(libcachemgr::log_cachemgr,
                    "cachemgr_t::find_symlinked_cache_directories(): failed to read symbolic link: '{}' error_code: {}",
                    entry.path(), this->_last_system_error);
                return false;
            }

            // if the symlink target does not start with the given symlink_target_prefix then ignore this entry
            if (symlink_target.compare(0, symlink_target_prefix.size(), symlink_target_prefix) != 0)
            {
                continue;
            }

            // the target of the symlink must be a directory
            if (!fs::is_directory(symlink_target, this->_last_system_error))
            {
                LOG_ERROR(libcachemgr::log_cachemgr,
                    "cachemgr_t::find_symlinked_cache_directories(): symlink target '{}' is not a directory", symlink_target);
                continue;
            }

            // abort on errors immediately
            if (this->_last_system_error)
            {
                LOG_ERROR(libcachemgr::log_cachemgr,
                    "cachemgr_t::find_symlinked_cache_directories(): failed to stat directory: '{}' error_code: {}",
                    entry.path(), this->_last_system_error);
                return false;
            }

            // add the symlinked directory to the list
            this->_mapped_cache_directories.emplace_back(mapped_cache_directory_t{
                .directory_type = directory_type_t::symbolic_link,
                .original_path = entry.path(),
                .target_path = symlink_target,
            });
        }
        else if (entry.is_directory(this->_last_system_error))
        {
            if (this->_last_system_error)
            {
                LOG_ERROR(libcachemgr::log_cachemgr,
                    "cachemgr_t::find_symlinked_cache_directories(): failed to stat directory: '{}' error_code: {}",
                    entry.path(), this->_last_system_error);
                return false;
            }

            // skip this entry if it can't be accessed, avoids unnecessary aborts on permission errors
            if (!os_utils::can_access_file(entry.path(), std::filesystem::perms::owner_exec))
            {
                LOG_DEBUG(libcachemgr::log_cachemgr,
                    "cachemgr_t::find_symlinked_cache_directories(): skipping inaccessible directory: '{}'",
                    entry.path());
                continue;
            }

            // TODO: before this code is enabled, we need the ability to discard certain directories
            //       based on the user configuration file
            //       this is because we can't receive the mount point location
            //       resulting in directories to be added to the list which shouldn't be there
            // DEV NOTE: this is actually the case on my system, I bind mount XDG_ directories to another physical disk
            // if (os_utils::is_mount_point(entry.path()))
            // {
            //     LOG_DEBUG(libcachemgr::log_cachemgr, "found mount point: '{}'", entry.path());
            //     // ignore the target path for mount points (as it can't be received easily)
            //     this->_mapped_cache_directories.emplace_back(mapped_cache_directory_t{
            //         .directory_type = directory_type_t::bind_mount,
            //         .original_path = entry.path(),
            //         .target_path = entry.path(),
            //     });
            // }
        }
    }

    return true;
}

bool cachemgr_t::find_symlinked_cache_directories(
    const std::initializer_list<std::string> &paths, const std::string &symlink_target_prefix) noexcept
{
    // clear previous results
    this->_mapped_cache_directories.clear();

    // iterate over all given paths and search for symlinked directories
    for (const auto &path : paths)
    {
        // don't clear the results from the previous search
        this->_clear_previous_list = false;

        // search the next path for symlinked directories
        if (!this->find_symlinked_cache_directories(path, symlink_target_prefix))
        {
            // abort if any of the searches failed
            return false;
        }
    }

    return true;
}

cachemgr_t::cache_mappings_compare_results_t cachemgr_t::compare_cache_mappings(
    const libcachemgr::cache_mappings_t &cache_mappings) const noexcept
{
    using cache_mappings_compare_result_t = cache_mappings_compare_results_t::cache_mappings_compare_result_t;
    cache_mappings_compare_results_t compare_results;

    // allow for fast lookup
    std::unordered_map<std::string_view, std::list<mapped_cache_directory_t>::const_iterator> unordered_cache_directories;
    for (auto it = this->_mapped_cache_directories.begin(); it != this->_mapped_cache_directories.end(); ++it)
    {
        unordered_cache_directories[it->original_path] = it;
    }

    // the given cache mapping list takes precedence,
    // avoid comparing entries which are not interesting to the user (according to the configuration file)
    for (const auto &cache_mapping : cache_mappings)
    {
        // find the corresponding entry in the unordered_cache_directories map
        const auto cache_directory_it = unordered_cache_directories.find(cache_mapping.source);

        /**
         * Lambda function to push the difference to the result list.
         *
         * @param cache_directory actual cache directory
         */
        const auto push_compare_result = [&compare_results, &cache_mapping](const mapped_cache_directory_t &cache_directory) {
            // push difference to the result list
            compare_results.add_result(cache_mappings_compare_result_t{
                // cache directory is actually this
                .actual = libcachemgr::cache_mapping_t{
                    .type = cache_mapping.type,
                    .source = cache_directory.original_path,
                    .target = cache_directory.target_path,
                },
                // cache directory expected to be this
                .expected = libcachemgr::cache_mapping_t{
                    .type = cache_mapping.type,
                    .source = cache_mapping.source,
                    .target = cache_mapping.target,
                },
            });
        };

        // cache directory found
        if (cache_directory_it != unordered_cache_directories.end())
        {
            // get reference to the cache directory entry
            const auto &cache_directory = *cache_directory_it->second;

            LOG_DEBUG(libcachemgr::log_cachemgr,
                "cachemgr_t::compare_cache_mappings(): cache_directory.target_path='{}', cache_mapping.target='{}'",
                cache_directory.target_path, cache_mapping.target);

            // compare if the target paths are equal
            if (cache_directory.target_path != cache_mapping.target)
            {
                // push difference to the result list
                push_compare_result(cache_directory);
            }
        }

        // cache directory not found
        else
        {
            LOG_DEBUG(libcachemgr::log_cachemgr,
                "cachemgr_t::compare_cache_mappings(): cache_directory.target_path=[NOT FOUND], cache_mapping.target='{}'",
                cache_mapping.target);

            // push difference to the result list
            push_compare_result(mapped_cache_directory_t{});
        }
    }

    return compare_results;
}
