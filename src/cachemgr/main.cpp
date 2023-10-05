#include <cstdio>
#include <string_view>
#include <array>

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

    /**
     * centralized version information and metadata about the program
     */
    struct program_metadata final
    {
        /**
         * application name
         */
        static constexpr std::string_view application_name = "cachemgr";

        /**
         * application version number
         *
         * adhere to semantic versioning 2.0.0, see https://semver.org/
         *
         * Q: When should I increment the major version number?
         * A: When there are user-facing breaking changes to the command line interface
         *    or the behavior of the program. `libcachemgr` is a private library and is
         *    not intended for public consumption, so it doesn't need to adhere to semver.
         *
         * TODO: add `-dirty` suffix to indicate that this is a dirty build
         */
        static constexpr std::string_view application_version = "0.0.0-dev";

        // TODO: read git repository information at build time using cmake
        static constexpr std::string_view git_branch = "";
        static constexpr std::string_view git_commit = "";
    };

    /**
     * compile-time command line option definition
     */
    struct cli_option
    {
        /// the type of the command line option
        using arg_type_t = argparse::Argument::Type;

        static constexpr auto boolean_type = arg_type_t::Boolean;
        static constexpr auto string_type = arg_type_t::String;

        /// compile-time construct a command line option definition
        constexpr cli_option(
            const char *long_opt,
            const char *short_opt,
            const char *description,
            const arg_type_t arg_type,
            const bool required = false)
        : long_opt(long_opt),
          short_opt(short_opt),
          description(description),
          arg_type(arg_type),
          required(required)
        {}

        const char *long_opt;
        const char *short_opt;
        const char *description;
        const arg_type_t arg_type;
        const bool required = false;

        /// this cli option has additional description which can only determined at runtime
        virtual bool has_runtime_description() const { return false; }

        /// get the additional runtime description of this cli option
        virtual std::string get_runtime_description() const { return ""; }

        /// implicit const char * conversion operator, returns the long option
        constexpr operator const char *() const {
            return long_opt;
        }

        /// implicit std::string conversion operator, returns the long option
        constexpr operator const std::string() const {
            return long_opt;
        }
    };

    /**
     * compile-time command line option definition for the `-c, --config` option
     */
    struct config_cli_option final : public cli_option
    {
        // inherit parent constructor
        using cli_option::cli_option;

        /// get the default configuration file location
        std::string get_config_file_location() const {
            static const std::string default_value = xdg_paths::get_xdg_config_home() + "/cachemgr.yaml";
            return default_value;
        }

        bool has_runtime_description() const override { return true; }
        std::string get_runtime_description() const override {
            return fmt::format("(defaults to {})", this->get_config_file_location());
        }
    };

    // command line options
    static constexpr const auto cli_opt_help =
        cli_option("help",      "h", "print this help message and exit", cli_option::boolean_type);
    static constexpr const auto cli_opt_version = // -v is reserved for verbose output
        cli_option("version",   "",  "print the version and exit",       cli_option::boolean_type);
    static constexpr const auto cli_opt_config =
        config_cli_option("config", "c", "path to the configuration file", cli_option::string_type);

    // array of command line options for easy registration in the parser
    static constexpr const std::array<const cli_option*, 3> cli_options = {
        &cli_opt_help,
        &cli_opt_version,
        &cli_opt_config,
    };
} // anonymous namespace

static int parse_cli_options(int argc, char **argv, bool *abort)
{
    using ArgType = argparse::Argument::Type;
    using ParseResult = argparse::ArgumentParser::Result;

    // prepare command line parser and options
    argparse::ArgumentParser parser(argc, argv);
    for (const auto *option : cli_options)
    {
        // get the full description of this option
        const std::string full_description = ([&option]{
            if (option->has_runtime_description()) {
                // append the additional runtime description to the existing description
                return std::string{option->description} + " " + option->get_runtime_description();
            } else {
                // return the existing description
                return std::string{option->description};
            }
        })();

        // register the option in the parser
        parser.addArgument(option->short_opt, option->long_opt, full_description, option->arg_type, option->required);
    }

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
        fmt::print("{} {}\n\n  Options:\n{}\n",
            program_metadata::application_name, program_metadata::application_version,
            parser.help());
        return 0;
    }

    // print application version and exit
    if (parser.exists(cli_opt_version))
    {
        *abort = true;
        fmt::print("{} {}\n", program_metadata::application_name, program_metadata::application_version);
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
    // catch errors early during first os_utils function calls
    // NOTE: parse_cli_options() and xdg_paths::get_xdg_cache_home() make calls to os_utils internally
    logging_helper::set_logger(std::make_shared<basic_utils_logger>());

    // parse command line arguments
    {
        bool abort = false;
        if (const auto status = parse_cli_options(argc, argv, &abort); abort)
        {
            // exit program with status if requested to abort
            return status;
        }
    }

    // initialize logging subsystem (includes os_utils function calls)
    libcachemgr::init_logging(libcachemgr::logging_config{
        // store the log file in $XDG_CACHE_HOME/cachemgr.log for now
        .log_file_path = xdg_paths::get_xdg_cache_home() + "/cachemgr.log",
    });

    return cachemgr_cli();
}
