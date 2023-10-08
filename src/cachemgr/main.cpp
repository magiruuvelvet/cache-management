#include <cstdio>
#include <string_view>
#include <array>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <utils/logging_helper.hpp>
#include <utils/os_utils.hpp>
#include <utils/types.hpp>
#include <utils/freedesktop/xdg_paths.hpp>

#include <libcachemgr/logging.hpp>
#include <libcachemgr/config.hpp>
#include <libcachemgr/cachemgr.hpp>
#include <libcachemgr/libcachemgr.hpp>

template<> struct fmt::formatter<human_readable_file_size> : ostream_formatter{};

#include <argparse/argparse.hpp>

using program_metadata = libcachemgr::program_metadata;
using configuration_t = libcachemgr::configuration_t;

static int cachemgr_cli()
{
    // parse the configuration file
    configuration_t::file_error file_error;
    configuration_t::parse_error parse_error;
    configuration_t config(libcachemgr::user_configuration()->configuration_file(), &file_error, &parse_error);

    // abort if there was an error parsing the configuration file
    if (file_error != configuration_t::file_error::no_error || parse_error != configuration_t::parse_error::no_error)
    {
        // errors are reported to the user via the logger
        return 1;
    }

    // create the cache manager
    cachemgr_t cachemgr;

    // find and validate all configured cache mappings
    if (const auto compare_results = cachemgr.find_mapped_cache_directories(config.cache_mappings()); compare_results)
    {
        LOG_WARNING(libcachemgr::log_cachemgr,
            "found {} differences between expected and actual cache mappings",
            compare_results.count());
    }

    if (libcachemgr::user_configuration()->show_usage_stats())
    {
        fmt::print("Calculating usage statistics...\n");

        std::uintmax_t total_size = 0;

        // used to pad the output
        std::uint32_t max_length_of_source_path = 0;
        std::uint32_t max_length_of_target_path = 0;

        // collect usage statistics and print the results of individual directories
        for (const auto &dir : cachemgr.mapped_cache_directories())
        {
            LOG_INFO(libcachemgr::log_main, "calculating usage statistics for directory: {}", dir.target_path);

            if (dir.original_path.size() > max_length_of_source_path)
            {
                max_length_of_source_path = dir.original_path.size();
            }
            if (dir.target_path.size() > max_length_of_target_path)
            {
                max_length_of_target_path = dir.target_path.size();
            }

            const auto [dir_size, ec] = os_utils::get_used_disk_space_of(dir.target_path);
            if (ec)
            {
                LOG_WARNING(libcachemgr::log_main, "failed to get used disk space of '{}': {}", dir.target_path, ec);
            }
            total_size += dir_size;
            dir.disk_size = dir_size;
        }
        for (const auto &dir : cachemgr.sorted_mapped_cache_directories())
        {
            const auto separator = dir->directory_type == cachemgr_t::directory_type_t::standalone ?
                "  " : "->";
            fmt::print("{:<{}} {} {:<{}} : {:>8} ({} bytes)\n",
                dir->original_path, max_length_of_source_path,
                separator,
                dir->target_path, max_length_of_target_path,
                human_readable_file_size{dir->disk_size}, dir->disk_size);
        }

        // print the total size of all cache directories
        fmt::print("{:>{}} total size : {:>8} ({} bytes)\n", " ",
            max_length_of_source_path + max_length_of_target_path - 7,
            human_readable_file_size{total_size}, total_size);

        return 0;
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
            static const std::string default_value = freedesktop::xdg_paths::get_xdg_config_home() + "/cachemgr.yaml";
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

    // print usage statistics of caches and exit program
    static constexpr const auto cli_opt_usage_stats =
        cli_option("usage", "u", "show the usage statistics of caches", cli_option::boolean_type);

    // array of command line options for easy registration in the parser
    static constexpr const std::array<const cli_option*, 4> cli_options = {
        &cli_opt_help,
        &cli_opt_version,
        &cli_opt_config,
        &cli_opt_usage_stats,
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
            program_metadata::application_name,
            program_metadata::full_application_version(),
            parser.help());
        return 0;
    }

    // print application version and exit
    if (parser.exists(cli_opt_version))
    {
        *abort = true;
        fmt::print("{} {}\n",
            program_metadata::application_name,
            program_metadata::full_application_version());
        return 0;
    }

    // obtain the location of the configuration file
    if (parser.exists(cli_opt_config))
    {
        libcachemgr::user_configuration()->set_configuration_file(parser.get<std::string>(cli_opt_config));
    }
    else
    {
        libcachemgr::user_configuration()->set_configuration_file(cli_opt_config.get_config_file_location());
    }

    // does the user want to show the usage statistics of the caches?
    if (parser.exists(cli_opt_usage_stats))
    {
        libcachemgr::user_configuration()->set_show_usage_stats(true);
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
    // NOTE: parse_cli_options() and freedesktop::xdg_paths::get_xdg_cache_home() make calls to os_utils internally
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
        // silence info logs in the console
        .log_level_console = quill::LogLevel::Warning,

        // store the log file in $XDG_CACHE_HOME/cachemgr.log for now
        .log_file_path = freedesktop::xdg_paths::get_xdg_cache_home() + "/cachemgr.log",
    });

    return cachemgr_cli();
}
