#include "logging.hpp"

#include <cstdlib>
#include <list>

#if !defined(PROJECT_PLATFORM_WINDOWS)
#include <csignal>
#endif

#include <quill/Quill.h>

#include "macros.hpp"
#include "libcachemgr.hpp"

#include <utils/logging_helper.hpp>
#include <utils/freedesktop/os-release.hpp>

extern "C" {

/**
 * Shutdown routines for the quill logging library.
 */
LIBCACHEMGR_ATTRIBUTE_USED
void shutdown_quill(void)
{
    quill::flush();
}

#if !defined(PROJECT_PLATFORM_WINDOWS)

static constexpr int signal_offset = 128;

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
LIBCACHEMGR_ATTRIBUTE_USED
void crash_signal_handler(int signal)
{
    // restore original signal handler
    std::signal(signal, SIG_DFL);

    fmt::print(stderr, "program received crashing signal {}, running shutdown routines...\n", signal);

    // shutdown quill logging thread
    shutdown_quill();

    // re-raise the signal and let the system handle it
    fmt::print(stderr, "re-raising original signal, goodbye.\n");
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
[[noreturn]] LIBCACHEMGR_ATTRIBUTE_USED
void normal_signal_handler(int signal)
{
    // call std::atexit functions before terminating the application
    std::exit(signal_offset + signal);
}

#endif // !defined(PROJECT_PLATFORM_WINDOWS)

} // extern "C"

namespace {

/**
 * Obtain log messages produced by the utils library.
 *
 * Log messages are forwarded to the quill logging library.
 */
class quill_utils_logger final : public logging_helper
{
public:
    quill_utils_logger(const libcachemgr::logging_config &config)
    {
        this->_quill_utils_logger = libcachemgr::create_logger("utils", config);
    }

    void log_debug(const std::string &message) override
    {
        LOG_DEBUG(this->_quill_utils_logger, "{}", message);
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

/// see quill::PatternFormatter#_set_pattern for available patterns
static constexpr const char *log_pattern = "%(level_id) [%(ascii_time)][%(thread_name)][%(logger_name:<8)] %(message)";
static constexpr const char *timestamp_pattern = "%Y-%m-%d %H:%M:%S.%Qns";

static constexpr std::uint8_t handler_id_console{0x8};
static constexpr std::uint8_t handler_id_file{0x16};

/// list of all loggers that have been created
static std::list<quill::Logger*> quill_loggers;

} // anonymous namespace

quill::Logger *libcachemgr::log_main = nullptr;
quill::Logger *libcachemgr::log_cachemgr = nullptr;
quill::Logger *libcachemgr::log_config = nullptr;
quill::Logger *libcachemgr::log_pm = nullptr;
  quill::Logger *libcachemgr::log_composer = nullptr;
  quill::Logger *libcachemgr::log_npm = nullptr;
quill::Logger *libcachemgr::log_db = nullptr;
quill::Logger *libcachemgr::log_test = nullptr;

void libcachemgr::init_logging(const logging_config &config)
{
#ifndef CACHEMGR_PROFILING_BUILD
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
    logging_helper::set_logger(std::make_shared<quill_utils_logger>(config));

    // create loggers
    log_main =     libcachemgr::create_logger("main", config);
    log_cachemgr = libcachemgr::create_logger("cachemgr", config);
    log_config =   libcachemgr::create_logger("config", config);
    log_pm =       libcachemgr::create_logger("pm", config);
      log_composer = libcachemgr::create_logger("composer", config);
      log_npm =      libcachemgr::create_logger("npm", config);
    log_db =       libcachemgr::create_logger("database", config);
    log_test =     libcachemgr::create_logger("test", config);

    LOG_INFO(log_main, "starting {} {}", program_metadata::application_name, program_metadata::full_application_version());

    if (config.log_os_release_on_startup)
    {
        std::string os_name, os_version;
        if (const freedesktop::os_release_t os_release; os_release.has_os_release())
        {
            os_name = os_release.unified_name();
            os_version = os_release.unified_version();
        }
        else
        {
            //LOG_WARNING(log_main, "could not obtain OS release information");
            os_name = program_metadata::platform_name;
        }

        LOG_INFO(log_main, "OS: {} {}", os_name, os_version);
    }
#endif // CACHEMGR_PROFILING_BUILD
}

void libcachemgr::change_log_level(const logging_config &config)
{
#ifndef CACHEMGR_PROFILING_BUILD
    for (auto &logger : quill_loggers)
    {
        // find the highest log level
        const quill::LogLevel highest_log_level = std::min(config.log_level_console, config.log_level_file);

        // change the log level of all handlers
        for (auto &handler : logger->handlers())
        {
            // identify the handler by its unique identifier
            switch (handler->get_handler_id())
            {
                case handler_id_console: handler->set_log_level(config.log_level_console); break;
                case handler_id_file:    handler->set_log_level(config.log_level_file); break;
            }
        }

        // change the master log level of the logger itself
        logger->set_log_level(highest_log_level);
    }
#endif // CACHEMGR_PROFILING_BUILD
}

void libcachemgr::flush_log()
{
#ifndef CACHEMGR_PROFILING_BUILD
    quill::flush();
#endif // CACHEMGR_PROFILING_BUILD
}

quill::Logger *libcachemgr::create_logger(const std::string &name, const logging_config &config)
{
    using ConsoleColors = quill::ConsoleColours;
    using LogLevel = quill::LogLevel;

    std::vector<std::shared_ptr<quill::Handler>> handlers;

    // find the highest log level
    const LogLevel highest_log_level = std::min(config.log_level_console, config.log_level_file);

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
        stdout_handler->set_pattern(log_pattern, timestamp_pattern);
        stdout_handler->set_log_level(config.log_level_console);
        stdout_handler->set_handler_id(handler_id_console);

        handlers.push_back(stdout_handler);
    }

    if (config.log_to_file)
    {
        // create file logger
        auto file_handler_config = quill::RotatingFileHandlerConfig();
        file_handler_config.set_pattern(log_pattern, timestamp_pattern);
        file_handler_config.set_rotation_naming_scheme(quill::RotatingFileHandlerConfig::RotationNamingScheme::Index);
        file_handler_config.set_rotation_max_file_size(4096 * 1024); // 4 MiB
        file_handler_config.set_remove_old_files(false);

        // create log file at the given location {config.log_level_file}
        auto file_handler = quill::rotating_file_handler(config.log_file_path, file_handler_config);
        file_handler->set_log_level(config.log_level_file);
        file_handler->set_handler_id(handler_id_file);

        handlers.push_back(file_handler);
    }

    // create loggers
    auto logger = quill::create_logger(name, std::move(handlers));
    // set the highest possible log level as the default log level,
    // individual handlers might have a lower log level than this level
    // this avoids running log formatters at runtime when they are not needed
    logger->set_log_level(highest_log_level);
    quill_loggers.emplace_back(logger);
    return logger;
}

quill::Logger *libcachemgr::get_logger(const std::string &name)
{
    return quill::get_logger(name.c_str());
}
