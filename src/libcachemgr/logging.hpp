#pragma once

#include <quill/Quill.h>

namespace libcachemgr {

extern quill::Logger *log_main;
extern quill::Logger *log_cachemgr;
extern quill::Logger *log_config;
extern quill::Logger *log_test;

/**
 * Configure logging during initialization.
 */
struct logging_config
{
    /**
     * Initializes the console logger.
     */
    bool log_to_console = true;

    /**
     * Initializes the file logger.
     */
    bool log_to_file = true;

    /**
     * The default log level for the console logger.
     */
    quill::LogLevel log_level_console = quill::LogLevel::Info; // TODO: should be configurable (CMake + command line)

    /**
     * The default log level for the file logger.
     */
    quill::LogLevel log_level_file = quill::LogLevel::Debug; // TODO: should be configurable (CMake + command line)

    /**
     * The log file path.
     */
    std::string log_file_path = "cachemgr.log"; // in working directory by default

    /**
     * Constructs a {utils::freedesktop::os_release_t} instance and logs its info upon logging startup.
     */
    bool log_os_release_on_startup = true;
};

/**
 * Initializes the logging subsystem.
 */
void init_logging(const logging_config &config = {});

/**
 * Create a new logger with the desires configuration.
 *
 * @param name name of the logger
 * @return
 */
quill::Logger *create_logger(const std::string &name, const logging_config &config = {});

/**
 * Get the logger with the given name.
 *
 * @param name name of the logger
 * @return
 */
quill::Logger *get_logger(const std::string &name);

} // namespace libcachemgr
