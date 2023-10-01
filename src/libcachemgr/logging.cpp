#include "logging.hpp"

#include <cstdlib>

#if !defined(PROJECT_PLATFORM_WINDOWS)
#include <csignal>
#endif

#include <quill/Quill.h>

#include <utils/logging_helper.hpp>
#include <utils/os_utils.hpp>

/**
 * Shutdown routines for the quill logging library.
 */
static void shutdown_quill()
{
    quill::flush();
}

#if !defined(PROJECT_PLATFORM_WINDOWS)

/**
 * Signal handler for application crashes.
 *
 *  - SIGSEGV (segmentation fault)
 *  - SIGABRT (abort)
 *  - SIGFPE (floating point exception)
 *  - SIGILL (illegal instruction)
 *
 * @param signal
 */
static void crash_signal_handler(int signal)
{
    // call std::atexit functions and re-raise the signal
    shutdown_quill();

    // restore original signal handler and re-raise the signal
    std::signal(signal, SIG_DFL);
    std::raise(signal);
}

/**
 * Signal handler for regular application termination.
 *
 *  - SIGINT (interrupt)
 *  - SIGTERM (terminate)
 *
 * @param signal
 */
static void normal_signal_handler(int signal)
{
    // call std::atexit functions before terminating the application
    std::exit(signal);
}

#endif // !defined(PROJECT_PLATFORM_WINDOWS)

namespace {

/**
 * Obtain log messages produced by the utils library.
 *
 * Log messages are forwarded to the quill logging library.
 */
class quill_utils_logger : public logging_helper
{
public:
    quill_utils_logger()
    {
        this->_quill_utils_logger = libcachemgr::create_logger("utils");
    }

    void log_info(const std::string &message) override
    {
        LOG_INFO(this->_quill_utils_logger, "{}", message);
    }

    void log_warning(const std::string &message) override
    {
        LOG_WARNING(this->_quill_utils_logger, "{}", message);
    }

    void log_error(const std::string &message) override
    {
        LOG_ERROR(this->_quill_utils_logger, "{}", message);
    }

private:
    quill::Logger *_quill_utils_logger = nullptr;
};

// directories needed for logging facilities
static std::string home_dir, xdg_cache_home;

/// see quill::PatternFormatter#_set_pattern for available patterns
static constexpr const char *log_pattern = "%(level_id) [%(ascii_time)][%(thread_name)][%(logger_name:<8)] %(message)";

} // anonymous namespace

quill::Logger *libcachemgr::log_main = nullptr;
quill::Logger *libcachemgr::log_cachemgr = nullptr;
quill::Logger *libcachemgr::log_config = nullptr;
quill::Logger *libcachemgr::log_test = nullptr;

void libcachemgr::init_logging(const logging_config &config)
{
    // receive essential directories
    home_dir = os_utils::get_home_directory();
    xdg_cache_home = os_utils::getenv("XDG_CACHE_HOME", home_dir + "/.cache");

    // configure quill logging
    quill::configure(([]{
        quill::Config cfg;
        cfg.enable_console_colours = true;
        return cfg;
    })());

    // start the logging thread
    quill::start(false);

    // register shutdown function for quill
    std::atexit(shutdown_quill);

    #if !defined(PROJECT_PLATFORM_WINDOWS)
    std::signal(SIGSEGV, crash_signal_handler);
    std::signal(SIGABRT, crash_signal_handler);
    std::signal(SIGFPE, crash_signal_handler);
    std::signal(SIGILL, crash_signal_handler);
    std::signal(SIGINT, normal_signal_handler);
    std::signal(SIGTERM, normal_signal_handler);
    #endif

    // register a logger for the utils library
    logging_helper::set_logger(std::make_shared<quill_utils_logger>());

    // create loggers
    log_main =     libcachemgr::create_logger("main", config);
    log_cachemgr = libcachemgr::create_logger("cachemgr", config);
    log_config =   libcachemgr::create_logger("config", config);
    log_test =     libcachemgr::create_logger("test", config);
}

quill::Logger *libcachemgr::create_logger(const std::string &name, const logging_config &config)
{
    using ConsoleColors = quill::ConsoleColours;
    using LogLevel = quill::LogLevel;

    std::vector<std::shared_ptr<quill::Handler>> handlers;

    if (config.log_to_console)
    {
        // setup colors for console logging
        ConsoleColors colors;
        colors.set_colour(LogLevel::Info, ConsoleColors::reset);
        colors.set_colour(LogLevel::Warning, ConsoleColors::yellow_bold);
        colors.set_colour(LogLevel::Error, ConsoleColors::red_bold);
        colors.set_colour(LogLevel::Critical, ConsoleColors::red_bold);
        colors.set_colour(LogLevel::Debug, ConsoleColors::white);

        // create stdout logger
        auto stdout_handler = quill::stdout_handler(name, colors);
        stdout_handler->set_pattern(log_pattern);

        handlers.push_back(stdout_handler);
    }

    if (config.log_to_file)
    {
        // create file logger
        auto file_handler_config = quill::FileHandlerConfig();
        file_handler_config.set_pattern(log_pattern);

        // create log file at $XDG_CACHE_HOME/cachemgr.log
        auto file_handler = quill::file_handler(xdg_cache_home + "/cachemgr.log", file_handler_config);

        handlers.push_back(file_handler);
    }

    // create loggers
    return quill::create_logger(name, std::move(handlers));
}

quill::Logger *libcachemgr::get_logger(const std::string &name)
{
    return quill::get_logger(name.c_str());
}
