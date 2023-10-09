#include "cachemgr.hpp"
#include "logging.hpp"

#include <filesystem>
#include <unordered_map>
#include <string_view>

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
        // TODO: handle errors properly
        std::error_code ec;

        // empty source means this entry only has a target directory
        if (mapping.source.empty())
        {
            // add source-less directory to list
            this->_mapped_cache_directories.emplace_back(mapped_cache_directory_t{
                .directory_type = directory_type_t::standalone,
                .original_path = {},
                .target_path = mapping.target,
            });
            continue;
        }

        // source is a symbolic link
        if (std::filesystem::is_symlink(mapping.source, ec))
        {
            // resolve the symbolic link
            const std::string symlink_target = fs::read_symlink(mapping.source, ec);

            if (symlink_target != mapping.target)
            {
                LOG_WARNING(libcachemgr::log_cachemgr,
                    "expected symlink target for source '{}' to be '{}', but found '{}' instead",
                    mapping.source, mapping.target, symlink_target);
                compare_results.add_result(cache_mappings_compare_result_t{
                    // cache directory is actually this
                    .actual = libcachemgr::cache_mapping_t{
                        .type = mapping.type,
                        .source = mapping.source,
                        .target = symlink_target,
                    },
                    // cache directory expected to be this
                    .expected = libcachemgr::cache_mapping_t{
                        .type = mapping.type,
                        .source = mapping.source,
                        .target = mapping.target,
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
                });
            }
        }

        // source is a directory
        else if (std::filesystem::is_directory(mapping.source, ec))
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
                        .source = mapping.source,
                        .target = mapping.source,
                    },
                    // cache directory expected to be this
                    .expected = libcachemgr::cache_mapping_t{
                        .type = mapping.type,
                        .source = mapping.source,
                        .target = mapping.target,
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
                });
            }
        }

        // source is invalid
        else
        {
            LOG_WARNING(libcachemgr::log_cachemgr,
                "source '{}' not found on disk or is inaccessible",
                mapping.source);
            compare_results.add_result(cache_mappings_compare_result_t{
                    // cache directory is actually this
                    .actual = libcachemgr::cache_mapping_t{
                        .type = mapping.type,
                        .source = {},
                        .target = {},
                    },
                    // cache directory expected to be this
                    .expected = libcachemgr::cache_mapping_t{
                        .type = mapping.type,
                        .source = mapping.source,
                        .target = mapping.target,
                    },
                });
            continue;
        }
    }

    return compare_results;
}

const std::list<const cachemgr_t::mapped_cache_directory_t*> cachemgr_t::sorted_mapped_cache_directories(
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
