#pragma once

#include <cstdint>

namespace datetime_utils {

/**
 * Returns the current system time as UNIX epoch time in UTC.
 */
std::uint64_t get_current_system_timestamp_in_utc();

} // namespace datetime_utils
