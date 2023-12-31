#pragma once

#include <string>
#include <memory>

/**
 * Implement this class to obtain logging messages generated by the library.
 *
 * Nothing is printed to stdout or stderr to allow developers to handle errors
 * their own way. By default all messages are sent to the void.
 */
class logging_helper
{
public:
    /**
     * Debugging messages.
     */
    virtual void log_debug(const std::string& message) = 0;

    /**
     * General informative messages for verbose logging.
     */
    virtual void log_info(const std::string &message) = 0;

    /**
     * Something didn't work as expected, but was handled gracefully by the library.
     */
    virtual void log_warning(const std::string &message) = 0;

    /**
     * Something is broken and needs to be handled by the library consumer.
     */
    virtual void log_error(const std::string &message) = 0;

private:
    // logging_helper instance singleton
    static std::shared_ptr<logging_helper> _logger;

protected:
    logging_helper() = default;
    virtual ~logging_helper() = default;

public:
    /**
     * Set this to your own logger implementation.
     */
    static inline void set_logger(std::shared_ptr<logging_helper> logger)
    {
        _logger = std::move(logger);
    }

    /**
     * Called by the library to log messages.
     */
    static inline logging_helper *get_logger()
    {
        return _logger.get();
    }
};
