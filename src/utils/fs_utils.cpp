#include "fs_utils.hpp"

#include <filesystem>
#include <fstream>
#include <cstring>
#include <regex>

#include "logging_helper.hpp"

namespace fs_utils {

std::string read_text_file(std::string_view path, std::error_code *ec) noexcept
{
    std::string buffer;

    std::ifstream fstream(path, std::ios::in);
    if (!fstream)
    {
        if (ec != nullptr)
        {
            (*ec) = std::make_error_code(std::errc{errno});
        }
        // TODO: use <format> once it is more available across different C++ standard libaries
        logging_helper::get_logger()->log_error("failed to open file: " + std::string{path} +
            " (" + strerror(errno) + ")");
        return {};
    }

    fstream.seekg(0, std::ios::end);
    buffer.resize(fstream.tellg()); // preallocate buffer
    fstream.seekg(0);
    fstream.read(buffer.data(), buffer.size());
    fstream.close();

    if (ec != nullptr)
    {
        // ensure any previous errors are cleared
        ec->clear();
    }

    return buffer;
}

std::error_code find_in_text_file(std::string_view path, std::string &out,
    std::function<bool(std::string_view line, std::string &cb_out)> line_callback) noexcept
{
    std::ifstream fstream(path, std::ios::in);
    if (!fstream)
    {
        // TODO: use <format> once it is more available across different C++ standard libaries
        logging_helper::get_logger()->log_error("failed to open file: " + std::string{path} +
            " (" + strerror(errno) + ")");
        return std::make_error_code(std::errc{errno});
    }

    if (fstream.is_open())
    {
        std::string line;
        while (std::getline(fstream, line))
        {
            std::string cb_out;
            if (line_callback(line, cb_out))
            {
                out = cb_out;
                break;
            }
        }

        fstream.close();
    }

    // empty error code indicates success
    return std::error_code{};
}

std::list<std::string> resolve_wildcard_pattern(const std::string &pattern, std::error_code *user_ec) noexcept
{
    // note: there is this glob library https://github.com/p-ranav/glob
    // but it does much more than just simple wildcard matching and
    // it uses exceptions for error handling, which is not desirable for this application

    namespace fs = std::filesystem;

    if (pattern.empty())
    {
        return {};
    }

    std::list<std::string> file_paths;
    std::error_code ec;

    const fs::path input_path(pattern);
    const fs::path directory = input_path.parent_path();
    const std::string file_wildcard_pattern = input_path.filename();
    const std::regex regex_pattern(std::regex_replace(file_wildcard_pattern, std::regex("\\*"), ".*"));

    if (fs::exists(directory, ec) && fs::is_directory(directory, ec))
    {
        for (const auto &entry : fs::directory_iterator(directory, fs::directory_options::skip_permission_denied, ec))
        {
            // check if the entry is a regular file and matches the wildcard pattern
            if (fs::is_regular_file(entry, ec))
            {
                const auto has_match = std::regex_match(entry.path().filename().string(), regex_pattern);

                logging_helper::get_logger()->log_info(
                    "matching file: '" + entry.path().string() + "' against pattern: " + file_wildcard_pattern +
                    " (" + (has_match ? "match" : "no match") + ")");

                if (has_match)
                {
                    file_paths.emplace_back(entry.path().string());
                }
            }
            else if (ec)
            {
                logging_helper::get_logger()->log_warning("skipping inaccessible entry: " + entry.path().string() +
                    " (" + strerror(errno) + ")");
                continue;
            }
        }
        if (ec)
        {
            logging_helper::get_logger()->log_error("failed to read directory: " + directory.string() +
                " (" + strerror(errno) + ")");
            if (user_ec != nullptr)
            {
                (*user_ec) = std::make_error_code(std::errc{errno});
            }
            return {};
        }
    }
    else if (ec)
    {
        logging_helper::get_logger()->log_error("failed to stat directory: " + directory.string() +
            " (" + strerror(errno) + ")");
        if (user_ec != nullptr)
        {
            (*user_ec) = std::make_error_code(std::errc{errno});
        }
        return {};
    }

    return file_paths;
}

} // namespace fs_utils
