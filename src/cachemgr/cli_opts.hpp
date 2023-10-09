#include <string>
#include <array>

#include <fmt/format.h>

#include <utils/freedesktop/xdg_paths.hpp>
#include <utils/types/pointer.hpp>

#include <argparse/argparse.hpp>

namespace {

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
static constexpr const std::array<observer_ptr<cli_option>, 4> cli_options = {
    &cli_opt_help,
    &cli_opt_version,
    &cli_opt_config,
    &cli_opt_usage_stats,
};

} // anonymous namespace
