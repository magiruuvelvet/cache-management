#pragma once

#include <string>
#include <optional>
#include <cstdint>

namespace libcachemgr {
namespace database {

struct cache_trend final
{
    /// UTC unix timestamp when the trend was calculated
    std::uint64_t timestamp;

    /// user-defined cache_mappings[].id
    std::string cache_mapping_id;

    /// name of the package manager
    std::optional<std::string> package_manager;

    /// cache size in bytes
    std::uintmax_t cache_size;
};

} // namespace database
} // namespace libcachemgr
