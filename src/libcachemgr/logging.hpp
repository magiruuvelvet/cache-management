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
};

/**
 * Initializes the logging system.
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
