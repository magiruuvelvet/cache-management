#include <cstdio>
#include <string>
#include <filesystem>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <utils/os_utils.hpp>
#include <utils/types/file_size_units.hpp>
#include <utils/freedesktop/xdg_paths.hpp>

#include <libcachemgr/logging.hpp>
#include <libcachemgr/config.hpp>
#include <libcachemgr/cachemgr.hpp>
#include <libcachemgr/libcachemgr.hpp>
#include <libcachemgr/package_manager_support/pm_registry.hpp>

#include "basic_utils_logger.hpp"
#include "cli_opts.hpp"

using human_readable_file_size = file_size_units::human_readable_file_size;
template<> struct fmt::formatter<human_readable_file_size> : ostream_formatter{};

using program_metadata = libcachemgr::program_metadata;
using configuration_t = libcachemgr::configuration_t;

/// small helper function to calculate disk usage and handle errors
static std::uintmax_t get_used_disk_space_of_safe(const std::string &path)
{
    static const auto log_warning = [&path](const std::error_code &ec){
        LOG_WARNING(libcachemgr::log_main, "failed to get used disk space of '{}': {}", path, ec);
    };

    std::error_code ec;

    // the given path is more likely to be a directory
    [[likely]] if (std::filesystem::is_directory(path, ec))
    {
        const auto [dir_size, ec_dir] = os_utils::get_used_disk_space_of(path);
        if (ec_dir)
        {
            log_warning(ec);
        }
        return dir_size;
    }
    else if (ec)
    {
        log_warning(ec);
    }

    else if (std::filesystem::is_regular_file(path, ec))
    {
        const auto file_size = std::filesystem::file_size(path, ec);
        if (ec)
        {
            log_warning(ec);
        }
        return file_size;
    }
    else if (ec)
    {
        log_warning(ec);
    }

    return 0;
}

static int cachemgr_cli()
{
    // parse the configuration file
    configuration_t::file_error file_error;
    configuration_t::parse_error parse_error;
    const configuration_t config(libcachemgr::user_configuration()->configuration_file(), &file_error, &parse_error);

    // abort if there was an error parsing the configuration file
    if (file_error != configuration_t::file_error::no_error || parse_error != configuration_t::parse_error::no_error)
    {
        // errors are reported to the user via the logger
        return 1;
    }

    // create the cache manager
    cachemgr_t cachemgr;

    // find and validate all configured cache mappings
    cachemgr_t::cache_mappings_compare_results_t::difference_size_type cache_mappings_difference = 0;
    if (const auto compare_results = cachemgr.find_mapped_cache_directories(config.cache_mappings()); compare_results)
    {
        cache_mappings_difference = compare_results.count();
        LOG_WARNING(libcachemgr::log_cachemgr,
            "found {} differences between expected and actual cache mappings",
            compare_results.count());
    }

    if (libcachemgr::user_configuration()->verify_cache_mappings())
    {
        return cache_mappings_difference > 0 ? 1 : 0;
    }

    else if (libcachemgr::user_configuration()->show_usage_stats())
    {
        fmt::print("Calculating usage statistics...\n");

        using libcachemgr::directory_type_t;

        std::uintmax_t total_size = 0;

        // used to pad the output
        using mcd_t = cachemgr_t::mapped_cache_directory_t;
        decltype(mcd_t::original_path)::size_type max_length_of_source_path = 0;
        decltype(mcd_t::target_path)::size_type max_length_of_target_path = 0;

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

            // only obtain used disk space if the target path is not empty
            if (dir.target_path.size() > 0 && dir.directory_type != directory_type_t::wildcard)
            {
                const auto dir_size = get_used_disk_space_of_safe(dir.target_path);
                total_size += dir_size;
                dir.disk_size = dir_size;
            }
            // obtain used disk space for a list of source files
            else if (dir.resolved_source_files.size() > 0)
            {
                for (const auto &source_file : dir.resolved_source_files)
                {
                    const auto file_size = get_used_disk_space_of_safe(source_file);
                    total_size += file_size;
                    dir.disk_size += file_size;
                }
            }
            else
            {
                LOG_ERROR(libcachemgr::log_main, "unreachable code reached");
            }
        }
        for (const auto &dir : cachemgr.sorted_mapped_cache_directories())
        {
            const auto separator = dir->directory_type ==
                directory_type_t::standalone || dir->directory_type == directory_type_t::wildcard ?
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

        // print the available space on the filesystem where cache_root resides
        const auto [available_disk_space, ec] = os_utils::get_available_disk_space_of(config.cache_root());
        if (ec)
        {
            LOG_WARNING(libcachemgr::log_main, "failed to get available disk space of '{}': {}", config.cache_root(), ec);
        }
        fmt::print("{:>{}} available space on cache root : {:>8} ({} bytes)\n", " ",
            max_length_of_source_path + max_length_of_target_path - 26,
            human_readable_file_size{available_disk_space}, available_disk_space);

        return 0;
    }

    else if (libcachemgr::user_configuration()->print_pm_cache_locations())
    {
        using pm_base = libcachemgr::package_manager_support::pm_base;
        using pm_registry = libcachemgr::package_manager_support::pm_registry;

        pm_base::pm_name_type::size_type max_length_of_package_manager = 0;

        for (const auto &pm : pm_registry::user_registry())
        {
            if (pm.first.size() > max_length_of_package_manager)
            {
                max_length_of_package_manager = pm.first.size();
            }
        }
        for (const auto &pm : pm_registry::user_registry())
        {
            // get the current contextual cache directory from the package manager
            const auto cache_directory_path = pm.second->get_cache_directory_path();

            std::error_code ec;
            std::string symlink_target, separator{"  "};
            if (std::filesystem::is_symlink(cache_directory_path, ec))
            {
                // the {cache_directory_path} might not match the user-configured cache directory
                //   example: a package manager allows a per-project cache directory
                symlink_target = std::filesystem::read_symlink(cache_directory_path, ec);
                separator = "->";
            }

            if (ec)
            {
                LOG_WARNING(libcachemgr::log_main, "failed to stat file '{}': {}", cache_directory_path, ec);
            }

            if (symlink_target.empty())
            {
                fmt::print("{:<{}} : {}\n",
                    pm.second->pm_name(), max_length_of_package_manager,
                    cache_directory_path);
            }
            else
            {
                fmt::print("{:<{}} : {} {} {}\n",
                    pm.second->pm_name(), max_length_of_package_manager,
                    cache_directory_path,
                    separator, symlink_target);
            }
        }

        return 0;
    }

    return 0;
}

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

    // FIXME: this should be improved in the future, maybe switch to a more advanced command line parser.
    // if this is true, the program will be executed with the determined action, otherwise exit it.
    std::uint8_t has_cli_actions = 0;

    // print cli help message and exit
    if (argc <= 1 || parser.exists(cli_opt_help))
    {
        *abort = true;
        fmt::print("{} {}\n\n  Options:\n{}\n",
            program_metadata::application_name,
            program_metadata::full_application_version(),
            parser.help());
        return 0;
    }

    // print application version and exit
    else if (parser.exists(cli_opt_version))
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

    // does the user want to verify cache mappings?
    if (parser.exists(cli_opt_verify_cache_mappings))
    {
        has_cli_actions += 1;
        libcachemgr::user_configuration()->set_verify_cache_mappings(true);
    }

    // does the user want to show the usage statistics of the caches?
    if (parser.exists(cli_opt_usage_stats))
    {
        has_cli_actions += 1;
        libcachemgr::user_configuration()->set_show_usage_stats(true);
    }

    // does the user want to print the predicted cache location of package managers?
    if (parser.exists(cli_opt_print_pm_cache_locations))
    {
        has_cli_actions += 1;
        libcachemgr::user_configuration()->set_print_pm_cache_locations(true);
    }

    // abort if no valid action was determined
    if (has_cli_actions == 0)
    {
        *abort = true;
        // be extra safe and assume argv[0] can somehow not exist
        const auto prog_name = argc > 0 ? argv[0] : program_metadata::application_name;
        fmt::print("no valid action specified, please run `{} --help` for available actions\n", prog_name);
        return 1;
    }
    else if (has_cli_actions > 1)
    {
        *abort = true;
        // be extra safe and assume argv[0] can somehow not exist
        const auto prog_name = argc > 0 ? argv[0] : program_metadata::application_name;
        fmt::print("multiple actions specified, please run `{} --help` for available actions\n", prog_name);
        return 1;
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
#ifndef CACHEMGR_PROFILING_BUILD
    // catch errors early during first os_utils function calls
    // NOTE: parse_cli_options() and freedesktop::xdg_paths::get_xdg_cache_home() make calls to os_utils internally
    logging_helper::set_logger(std::make_shared<basic_utils_logger>());
#endif

    // parse command line arguments
    {
        bool abort = false;
        if (const auto status = parse_cli_options(argc, argv, &abort); abort)
        {
            // exit program with status if requested to abort
            return status;
        }
    }

#ifndef CACHEMGR_PROFILING_BUILD
    // initialize logging subsystem (includes os_utils function calls)
    libcachemgr::init_logging(libcachemgr::logging_config{
        // silence info logs in the console
        .log_level_console = quill::LogLevel::Warning,

        // store the log file in $XDG_CACHE_HOME/cachemgr.log for now
        .log_file_path = freedesktop::xdg_paths::get_xdg_cache_home() + "/cachemgr.log",
    });
#endif

    return cachemgr_cli();
}
