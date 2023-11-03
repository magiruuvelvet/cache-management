#include "datetime_utils.hpp"

#include <chrono>

namespace datetime_utils {

std::uint64_t get_current_system_timestamp_in_utc()
{
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

} // namespace datetime_utils
