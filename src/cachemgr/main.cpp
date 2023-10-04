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

static int parse_cli_options(int argc, char **argv, bool *abort)
{
    using ArgType = argparse::Argument::Type;
    using ParseResult = argparse::ArgumentParser::Result;

    // prepare command line parser and options
    argparse::ArgumentParser parser(argc, argv);
    parser.addArgument("h", cli_opt_help,    "print this help message and exit", ArgType::Boolean);
    parser.addArgument("",  cli_opt_version, "print the version and exit", ArgType::Boolean);

    // parse command line arguments
    switch (const auto parse_result = parser.parse())
    {
        // success is the most likely case, allow the compiler to optimize for it
        // break out of the switch statement as no error handling is required
        [[likely]] case ParseResult::Success: break;

        // handle error cases

        // not possible because there are no required arguments in the parser
        [[unlikely]] case ParseResult::InsufficientArguments:
        [[unlikely]] case ParseResult::MissingArgument:
        // not possible since this is just the default result code upon {argparse::ArgumentParser} initialization
        [[unlikely]] case ParseResult::Unknown:
        {
            // abort on any error
            *abort = true;
            // TODO: error messages
            return 2;
            break;
        }
    }

    // print cli help message and exit
    if (parser.exists(cli_opt_help))
    {
        *abort = true;
        fmt::print("{}\n", parser.help());
        return 0;
    }

    // print application version and exit
    if (parser.exists(cli_opt_version))
    {
        *abort = true;
        fmt::print("cachemgr 0.0.0\n"); // TODO: app versioning
        return 0;
    }

    return 0;
}

/**
 * Perform the initialization and execution of the CLI application.
 *  - parse command line arguments and populate the global state
 *  - initialize the logging subsystem
 *  - start the CLI application {cachemgr_cli}
 */
int main(int argc, char **argv)
{
    // parse command line arguments
    {
        bool abort = false;
        if (const auto status = parse_cli_options(argc, argv, &abort); abort)
        {
            // exit program with status if requested to abort
            return status;
        }
    }

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
