#include "logging_helper.hpp"

namespace {

/**
 * Sends all log messages it receives into the void.
 * Default logger for the library.
 */
class void_logger final : public logging_helper
{
    // since nothing happens with the log messages, we don't care about thread safety
public:
    constexpr inline void log_debug(const std::string &) override {}
    constexpr inline void log_info(const std::string &) override {}
    constexpr inline void log_warning(const std::string &) override {}
    constexpr inline void log_error(const std::string &) override {}
};

} // anonymous namespace

// initialize the logging_helper singleton with the void_logger
std::shared_ptr<logging_helper> logging_helper::_logger = std::make_shared<void_logger>();
