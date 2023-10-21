#include "cachemgr.hpp"
#include "logging.hpp"

#include <filesystem>
#include <unordered_map>
#include <string_view>
#include <algorithm>

#include <utils/fs_utils.hpp>
#include <utils/os_utils.hpp>

namespace fs = std::filesystem;

using libcachemgr::directory_type_t;

cachemgr_t::cachemgr_t()
{
}

cachemgr_t::~cachemgr_t()
{
}

cachemgr_t::cache_mappings_compare_results_t cachemgr_t::find_mapped_cache_directories(
    const libcachemgr::configuration_t::cache_mappings_t &cache_mappings) noexcept
{
    using cache_mappings_compare_result_t = cache_mappings_compare_results_t::cache_mappings_compare_result_t;

    // clear previous results
    this->_mapped_cache_directories.clear();

    cache_mappings_compare_results_t compare_results;

    for (const auto &mapping : cache_mappings)
    {
        // empty source means this entry only has a target directory
        if (mapping.type == directory_type_t::standalone)
        {
            // add source-less directory to list
            this->_mapped_cache_directories.emplace_back(libcachemgr::mapped_cache_directory_t{
                .directory_type = directory_type_t::standalone,
                .original_path = {}, // standalone doesn't have an original path
                .target_path = mapping.target,
                .package_manager = mapping.package_manager,
            });
        }

        // resolve wildcard pattern into a list of files
        else if (mapping.type == directory_type_t::wildcard)
        {
            std::error_code ec_wildcard_resolve;
            const auto resolved_files = fs_utils::resolve_wildcard_pattern(mapping.target, &ec_wildcard_resolve);

            if (ec_wildcard_resolve)
            {
                LOG_WARNING(libcachemgr::log_cachemgr,
                    "failed to resolve wildcard pattern for target '{}': {}",
                    mapping.target, ec_wildcard_resolve);
            }
            else
            {
                this->_mapped_cache_directories.emplace_back(libcachemgr::mapped_cache_directory_t{
                    .directory_type = directory_type_t::wildcard,
                    // wildcard patterns don't have an original path and are similar to standalone directories
                    .original_path = {},
                    .target_path = {}, // remove the target path as it is not technically a valid path
                    .package_manager = mapping.package_manager,
                    .resolved_source_files = resolved_files,
                    .wildcard_pattern = mapping.target, // preserve the wildcard pattern
                });
            }
        }

        // source must be a symbolic link
        else if (mapping.type == directory_type_t::symbolic_link)
        {
            std::string symlink_target;
            std::error_code ec_is_symlink, ec_read_symlink;
            bool is_valid = true;

            // source is not a symbolic link
            if (!std::filesystem::is_symlink(mapping.source, ec_is_symlink))
            {
                is_valid = false;
                LOG_WARNING(libcachemgr::log_cachemgr,
                    "expected source '{}' to be a symbolic link, but it isn't", mapping.source);
            }
            else if (ec_is_symlink)
            {
                is_valid = false;
                LOG_WARNING(libcachemgr::log_cachemgr,
                    "(is_symlink) failed to stat file '{}': {}", mapping.source, ec_is_symlink);
            }
            else
            {
                // resolve the symbolic link
                symlink_target = fs::read_symlink(mapping.source, ec_read_symlink);
            }

            if (ec_read_symlink)
            {
                is_valid = false;
                LOG_WARNING(libcachemgr::log_cachemgr,
                    "failed to read symbolic link '{}': {}", mapping.source, ec_read_symlink);
            }
            else if (symlink_target != mapping.target)
            {
                is_valid = false;
                LOG_WARNING(libcachemgr::log_cachemgr,
                    "expected symbolic link target for source '{}' to be '{}', but found '{}' instead",
                    mapping.source, mapping.target, symlink_target);
            }

            if (!is_valid)
            {
                compare_results.add_result(cache_mappings_compare_result_t{
                    // cache directory is actually this
                    .actual = libcachemgr::configuration_t::cache_mapping_t{
                        .id = mapping.id,
                        .type = mapping.type,
                        .package_manager = mapping.package_manager,
                        .source = mapping.source,
                        .target = symlink_target,
                    },
                    // cache directory expected to be this
                    .expected = libcachemgr::configuration_t::cache_mapping_t(mapping),
                });
            }
            else
            {
                // add the symlinked directory to the list
                this->_mapped_cache_directories.emplace_back(libcachemgr::mapped_cache_directory_t{
                    .directory_type = directory_type_t::symbolic_link,
                    .original_path = mapping.source,
                    .target_path = symlink_target,
                    .package_manager = mapping.package_manager,
                });
            }
        }

        // source must be a directory
        else if (mapping.type == directory_type_t::bind_mount)
        {
            std::error_code ec_is_directory;
            bool is_valid = true;

            // source is not a directory
            if (!std::filesystem::is_directory(mapping.source, ec_is_directory))
            {
                is_valid = false;
                LOG_WARNING(libcachemgr::log_cachemgr,
                    "expected source '{}' to be a directory, but it isn't", mapping.source);
            }
            else if (ec_is_directory)
            {
                is_valid = false;
                LOG_WARNING(libcachemgr::log_cachemgr,
                    "(is_directory) failed to stat file '{}': {}", mapping.source, ec_is_directory);
            }
            else if (!os_utils::is_mount_point(mapping.source))
            {
                is_valid = false;

                // expected directory to be a mount point
                LOG_WARNING(libcachemgr::log_cachemgr,
                    "expected directory '{}' to be a mount point, but found a regular directory instead",
                    mapping.source);
            }

            if (!is_valid)
            {
                compare_results.add_result(cache_mappings_compare_result_t{
                    // cache directory is actually this
                    .actual = libcachemgr::configuration_t::cache_mapping_t{
                        .id = mapping.id,
                        .type = mapping.type,
                        .package_manager = mapping.package_manager,
                        .source = mapping.source,
                        .target = mapping.source,
                    },
                    // cache directory expected to be this
                    .expected = libcachemgr::configuration_t::cache_mapping_t(mapping),
                });
            }
            else
            {
                // add the bind mount to the list
                this->_mapped_cache_directories.emplace_back(libcachemgr::mapped_cache_directory_t{
                   .directory_type = directory_type_t::bind_mount,
                   .original_path = mapping.source,
                   // treat the target path as the mount point location
                   .target_path = mapping.source,
                   .package_manager = mapping.package_manager,
                });
            }
        }
    }

    return compare_results;
}

const std::list<observer_ptr<libcachemgr::mapped_cache_directory_t>> cachemgr_t::sorted_mapped_cache_directories(
    sort_behavior sort_behavior) const noexcept
{
    std::list<const libcachemgr::mapped_cache_directory_t*> sorted_list;

    for (const auto &cache_dir : this->_mapped_cache_directories)
    {
        sorted_list.emplace_back(&cache_dir);
    }

    if (sort_behavior == sort_behavior::unsorted)
    {
        return sorted_list;
    }
    else if (sort_behavior == sort_behavior::disk_usage_descending)
    {
        sorted_list.sort([](const auto *lhs, const auto *rhs){
            return lhs->disk_size > rhs->disk_size;
        });
    }
    else if (sort_behavior == sort_behavior::disk_usage_ascending)
    {
        sorted_list.sort([](const auto *lhs, const auto *rhs){
            return lhs->disk_size < rhs->disk_size;
        });
    }

    return sorted_list;
}

const observer_ptr<libcachemgr::mapped_cache_directory_t> cachemgr_t::find_mapped_cache_directory_for_package_manager(
        libcachemgr::package_manager_support::pm_base::pm_name_type pm_name) const noexcept
{
    const auto it = std::find_if(this->_mapped_cache_directories.cbegin(), this->_mapped_cache_directories.cend(),
        [&pm_name](const libcachemgr::mapped_cache_directory_t &cache_mapping){
            return cache_mapping.package_manager && cache_mapping.package_manager()->pm_name() == pm_name;
        });

    if (it != this->_mapped_cache_directories.cend())
    {
        return &(*it);
    }
    else
    {
        return nullptr;
    }
}
