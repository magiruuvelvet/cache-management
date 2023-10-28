#pragma once

// log messages which require pluralization rules
// the log file only supports English, no need for a i18n library

#include <string_view>

#include "logging.hpp"

namespace libcachemgr {
namespace messages {

template<typename LoggerType, typename FileCountType, typename... Args>
inline constexpr auto LOG_CALCULATING_USAGE_STATISTICS_FOR_WILDCARD_PATTERN_WITH_FILE_COUNT(
    const LoggerType &logger, const FileCountType &file_count, Args&&... args)
{
    if (file_count == 1)
    {
        LOG_INFO(logger, "calculating usage statistics for wildcard pattern: {} ({} file)", args...);
    }
    else
    {
        LOG_INFO(logger, "calculating usage statistics for wildcard pattern: {} ({} files)", args...);
    }
}

} // namespace messages
} // namespace libcachemgr
