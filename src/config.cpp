#include "config.hpp"

#include <cstdio>
#include <fstream>
#include <filesystem>
#include <string_view>

#include <ryml.hpp>

/// yaml key names to avoid typos and repetitive strings in code
namespace {
    /// sequence of cache mappings
    /// cache_mappings:
    ///   - source: original cache location
    ///     target: symlinked cache location inside the compressed filesystem
    constexpr const char *key_seq_cache_mappings = "cache_mappings";
    constexpr const char *key_str_source = "source";
    constexpr const char *key_str_target = "target";
}

configuration_t::configuration_t(const std::string &config_file, file_error *file_error, parse_error *parse_error)
{
    // assume no errors at the beginning
    if (file_error != nullptr) { *file_error = file_error::no_error; }
    if (parse_error != nullptr) { *parse_error = parse_error::no_error; }

    std::error_code ec;

    // implicit file creation not supported, abort if the file doesn't exist
    if (!std::filesystem::exists(config_file, ec))
    {
        std::fprintf(stderr, "configuration file '%s' does not exist or is not accessible: %s\n",
            config_file.c_str(), ec.message().c_str());
        if (file_error != nullptr) { *file_error = file_error::not_found; }
        return;
    }

    // validate if the given file is a regular file (or a symbolic link to a regular file)
    if (!std::filesystem::is_regular_file(config_file, ec))
    {
        std::fprintf(stderr, "configuration file '%s' is not a regular file: %s\n",
            config_file.c_str(), ec.message().c_str());
        if (file_error != nullptr) { *file_error = file_error::not_a_file; }
        return;
    }

    // read the configuration file into memory
    std::string buffer;
    {
        // TODO: handle read errors
        std::ifstream config_file_stream(config_file, std::ios::in);
        config_file_stream.seekg(0, std::ios::end);
        buffer.resize(config_file_stream.tellg());
        config_file_stream.seekg(0);
        config_file_stream.read(buffer.data(), buffer.size());
        config_file_stream.close();
    }

    // parse the configuration file
    // TODO: handle parse errors (?)
    const ryml::Tree tree = ryml::parse_in_arena(buffer.c_str());

    // validate that 'cache_mappings' exists
    if (!tree.has_child(tree.root_id(), key_seq_cache_mappings))
    {
        std::fprintf(stderr, "'cache_mappings' not found\n");
        if (parse_error != nullptr) { *parse_error = parse_error::cache_mappings_seq_not_found; }
        return;
    }

    // get 'cache_mappings' sequence
    const auto &cache_mappings = tree[key_seq_cache_mappings];

    // validate that 'cache_mappings' is actually a sequence
    if (!cache_mappings.is_seq())
    {
        std::fprintf(stderr, "'cache_mappings' is not a sequence\n");
        if (parse_error != nullptr) { *parse_error = parse_error::cache_mappings_not_a_seq; }
        return;
    }
}
