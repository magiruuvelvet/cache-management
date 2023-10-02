#pragma once

#include <string>
#include <list>

namespace libcachemgr {

/**
 * Represents a cache mapping in the configuration file.
 */
struct cache_mapping_t
{
    const std::string type; // unused for now, type might change in the future
    const std::string source;
    const std::string target;
};

/**
 * List of cache mappings in the configuration file.
 */
using cache_mappings_t = std::list<cache_mapping_t>;

} // namespace libcachemgr
