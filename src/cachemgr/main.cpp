#include <cstdio>

#include <fmt/printf.h>

#include <utils/logging_helper.hpp>
#include <utils/os_utils.hpp>
#include <utils/xdg_paths.hpp>

#include <libcachemgr/logging.hpp>
#include <libcachemgr/config.hpp>
#include <libcachemgr/cachemgr.hpp>

#include <argparse/argparse.hpp>

static int cachemgr_cli()
{
    cachemgr_t cachemgr;

    // receive essential directories
    const auto home_dir = os_utils::get_home_directory();
    const auto xdg_cache_home = xdg_paths::get_xdg_cache_home();

    // scan HOME and XDG_CACHE_HOME for symlinked cache directories
    const bool result = cachemgr.find_symlinked_cache_directories({home_dir, xdg_cache_home}, "/caches/1000");

    std::printf("result = %s\n", result ? "true" : "false");

    for (auto&& dir : cachemgr.symlinked_cache_directories())
    {
        std::printf("%s -> %s\n", dir.original_path.c_str(), dir.target_path.c_str());
    }

    std::printf("is_mount_point(/home): %i\n", os_utils::is_mount_point("/home"));
    std::printf("is_mount_point(/caches/1000): %i\n", os_utils::is_mount_point("/caches/1000"));

    configuration_t::file_error file_error;
    configuration_t::parse_error parse_error;
    configuration_t config("./test.yaml", &file_error, &parse_error);

    std::printf("file_error = %i, parse_error = %i\n", file_error, parse_error);

    const auto compare_results = cachemgr.compare_cache_mappings(config.cache_mappings());
    if (compare_results)
    {
        LOG_WARNING(libcachemgr::log_main, "found differences between expected and actual cache mappings");

        LOG_WARNING(libcachemgr::log_main, "count of differences: {}", compare_results.count());

        for (const auto &diff : compare_results.differences())
        {
            LOG_WARNING(libcachemgr::log_main, "difference: {} <=> {}",
                diff.actual.target, diff.expected.target);
        }
    }

    return 0;
}

namespace {
    /// basic console logger for early program initialization
    /// used until the logging subsystem is initialized
    class basic_utils_logger : public logging_helper
    {
    public:
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

    static constexpr const char *cli_opt_help = "help";
    static constexpr const char *cli_opt_version = "version";
} // anonymous namespace

/**
 * Perform the initialization of the CLI application.
 *  - initialize the logging subsystem
 *  - parse command line arguments
 *
 * After completion, invoke {cachemgr_cli}.
 */
int main(int argc, char **argv)
{
    {
        // parse command line arguments
        argparse::ArgumentParser parser(argc, argv);
        using ArgType = argparse::Argument::Type;
        parser.addArgument("h", cli_opt_help,    "print this help message and exit", ArgType::Boolean);
        parser.addArgument("",  cli_opt_version, "print the version and exit", ArgType::Boolean);

        switch (const auto parse_result = parser.parse())
        {
            using ParseResult = argparse::ArgumentParser::Result;

            case ParseResult::Success:
                if (parser.exists(cli_opt_help))
                {
                    fmt::print("{}\n", parser.help());
                    return 0;
                }
                break;

            case ParseResult::InsufficientArguments:
                break;

            case ParseResult::MissingArgument:
                break;

            case ParseResult::Unknown:
                break;
        }
    } // discard command line parser and continue with the rest of the program

    // catch errors early during first os_utils function calls
    // NOTE: xdg_paths::get_xdg_cache_home() makes calls to os_utils internally
    logging_helper::set_logger(std::make_shared<basic_utils_logger>());

    // initialize logging subsystem (includes os_utils function calls)
    libcachemgr::init_logging(libcachemgr::logging_config{
        // store the log file in $XDG_CACHE_HOME/cachemgr.log for now
        .log_file_path = xdg_paths::get_xdg_cache_home() + "/cachemgr.log",
    });

    return cachemgr_cli();
}
