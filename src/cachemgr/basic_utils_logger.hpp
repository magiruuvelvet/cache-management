#pragma once

#include <string>

#include <fmt/format.h>

#include <utils/logging_helper.hpp>

namespace {
    /// basic console logger for early program initialization
    /// used until the logging subsystem is initialized
    class basic_utils_logger final : public logging_helper
    {
    public:
        void log_debug(const std::string &message) override {
            fmt::print(stderr, "[dbg] {}\n", message);
        }
        void log_info(const std::string &message) override {
            fmt::print(stderr, "[inf] {}\n", message);
        }
        void log_warning(const std::string &message) override {
            fmt::print(stderr, "[wrn] {}\n", message);
        }
        void log_error(const std::string &message) override {
            fmt::print(stderr, "[err] {}\n", message);
        }
    };
} // anonymous namespace
