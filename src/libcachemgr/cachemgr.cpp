#include "cachemgr.hpp"
#include "logging.hpp"

#include <filesystem>
#include <unordered_map>
#include <string_view>
#include <algorithm>

#include <utils/fs_utils.hpp>
#include <utils/os_utils.hpp>

namespace fs = std::filesystem;

cachemgr_t::cachemgr_t()
{
}

cachemgr_t::~cachemgr_t()
{
}

cachemgr_t::cache_mappings_compare_results_t cachemgr_t::find_mapped_cache_directories(
    const libcachemgr::cache_mappings_t &cache_mappings) noexcept
{
    using cache_mappings_compare_result_t = cache_mappings_compare_results_t::cache_mappings_compare_result_t;

    // clear previous results
    this->_mapped_cache_directories.clear();

    cache_mappings_compare_results_t compare_results;

    for (const auto &mapping : cache_mappings)
    {
        std::error_code ec_is_symlink, ec_is_directory;

        // empty source means this entry only has a target directory
        if (mapping.source.empty())
        {
            // add source-less directory to list
            this->_mapped_cache_directories.emplace_back(mapped_cache_directory_t{
                .directory_type = directory_type_t::standalone,
                .original_path = {},
                .target_path = mapping.target,
                .package_manager = mapping.package_manager,
            });
            continue;
        }

        // resolve wildcard pattern into a list of files
        else if (mapping.has_wildcard_matching)
        {
            std::error_code ec_wildcard_resolve;
            const auto resolved_files = fs_utils::resolve_wildcard_pattern(mapping.source, &ec_wildcard_resolve);

            if (ec_wildcard_resolve)
            {
                LOG_WARNING(libcachemgr::log_cachemgr,
                    "failed to resolve wildcard pattern for source '{}': {}",
                    mapping.source, ec_wildcard_resolve);
                continue;
            }
            else
            {
                this->_mapped_cache_directories.emplace_back(mapped_cache_directory_t{
                    .directory_type = directory_type_t::wildcard,
                    .original_path = mapping.source,
                    .target_path = {},
                    .package_manager = mapping.package_manager,
                    .resolved_source_files = resolved_files,
                });
            }
        }

        // source is a symbolic link
        else if (std::filesystem::is_symlink(mapping.source, ec_is_symlink))
        {
            // resolve the symbolic link
            std::error_code ec_read_symlink;
            const std::string symlink_target = fs::read_symlink(mapping.source, ec_read_symlink);

            if (ec_read_symlink)
            {
                LOG_WARNING(libcachemgr::log_cachemgr,
                    "failed to read symbolic link '{}': {}", mapping.source, ec_read_symlink);
            }

            if (symlink_target != mapping.target)
            {
                LOG_WARNING(libcachemgr::log_cachemgr,
                    "expected symlink target for source '{}' to be '{}', but found '{}' instead",
                    mapping.source, mapping.target, symlink_target);
                compare_results.add_result(cache_mappings_compare_result_t{
                    // cache directory is actually this
                    .actual = libcachemgr::cache_mapping_t{
                        .type = mapping.type,
                        .package_manager = mapping.package_manager,
                        .source = mapping.source,
                        .target = symlink_target,
                        .has_wildcard_matching = mapping.has_wildcard_matching,
                    },
                    // cache directory expected to be this
                    .expected = libcachemgr::cache_mapping_t{
                        .type = mapping.type,
                        .package_manager = mapping.package_manager,
                        .source = mapping.source,
                        .target = mapping.target,
                        .has_wildcard_matching = mapping.has_wildcard_matching,
                    },
                });
                continue;
            }
            else
            {
                // add the symlinked directory to the list
                this->_mapped_cache_directories.emplace_back(mapped_cache_directory_t{
                    .directory_type = directory_type_t::symbolic_link,
                    .original_path = mapping.source,
                    .target_path = symlink_target,
                    .package_manager = mapping.package_manager,
                });
            }
        }

        // source is a directory
        else if (std::filesystem::is_directory(mapping.source, ec_is_directory))
        {
            if (!os_utils::is_mount_point(mapping.source))
            {
                // expected directory to be a mount point
                LOG_WARNING(libcachemgr::log_cachemgr,
                    "expected directory '{}' to be a mount point, but found a regular directory instead",
                    mapping.source);
                compare_results.add_result(cache_mappings_compare_result_t{
                    // cache directory is actually this
                    .actual = libcachemgr::cache_mapping_t{
                        .type = mapping.type,
                        .package_manager = mapping.package_manager,
                        .source = mapping.source,
                        .target = mapping.source,
                        .has_wildcard_matching = mapping.has_wildcard_matching,
                    },
                    // cache directory expected to be this
                    .expected = libcachemgr::cache_mapping_t{
                        .type = mapping.type,
                        .package_manager = mapping.package_manager,
                        .source = mapping.source,
                        .target = mapping.target,
                        .has_wildcard_matching = mapping.has_wildcard_matching,
                    },
                });
                continue;
            }
            else
            {
                // add the bind mount to the list
                this->_mapped_cache_directories.emplace_back(mapped_cache_directory_t{
                   .directory_type = directory_type_t::bind_mount,
                   .original_path = mapping.source,
                   // treat the target path as the mount point location
                   .target_path = mapping.source,
                   .package_manager = mapping.package_manager,
                });
            }
        }

        // source is invalid
        else
        {
            if (ec_is_symlink)
            {
                LOG_WARNING(libcachemgr::log_cachemgr,
                    "(is_symlink) failed to stat file '{}': {}", mapping.source, ec_is_symlink);
            }
            else if (ec_is_directory)
            {
                LOG_WARNING(libcachemgr::log_cachemgr,
                    "(is_directory) failed to stat file '{}': {}", mapping.source, ec_is_directory);
            }

            LOG_WARNING(libcachemgr::log_cachemgr,
                "source '{}' was not found on disk or is inaccessible, check previous warnings for details",
                mapping.source);
            compare_results.add_result(cache_mappings_compare_result_t{
                    // cache directory is actually this
                    .actual = libcachemgr::cache_mapping_t{
                        .type = mapping.type,
                        .package_manager = mapping.package_manager,
                        .source = {},
                        .target = {},
                        .has_wildcard_matching = mapping.has_wildcard_matching,
                    },
                    // cache directory expected to be this
                    .expected = libcachemgr::cache_mapping_t{
                        .type = mapping.type,
                        .package_manager = mapping.package_manager,
                        .source = mapping.source,
                        .target = mapping.target,
                        .has_wildcard_matching = mapping.has_wildcard_matching,
                    },
                });
            continue;
        }
    }

    return compare_results;
}

const std::list<observer_ptr<cachemgr_t::mapped_cache_directory_t>> cachemgr_t::sorted_mapped_cache_directories(
    sort_behavior sort_behavior) const noexcept
{
    std::list<const cachemgr_t::mapped_cache_directory_t*> sorted_list;

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
        sorted_list.sort([](const mapped_cache_directory_t *lhs, const mapped_cache_directory_t *rhs){
            return lhs->disk_size > rhs->disk_size;
        });
    }
    else if (sort_behavior == sort_behavior::disk_usage_ascending)
    {
        sorted_list.sort([](const mapped_cache_directory_t *lhs, const mapped_cache_directory_t *rhs){
            return lhs->disk_size < rhs->disk_size;
        });
    }

    return sorted_list;
}

const observer_ptr<cachemgr_t::mapped_cache_directory_t> cachemgr_t::find_mapped_cache_directory_for_package_manager(
        libcachemgr::package_manager_support::pm_base::pm_name_type pm_name) const noexcept
{
    const auto it = std::find_if(this->_mapped_cache_directories.cbegin(), this->_mapped_cache_directories.cend(),
        [&pm_name](const mapped_cache_directory_t &cache_mapping){
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
