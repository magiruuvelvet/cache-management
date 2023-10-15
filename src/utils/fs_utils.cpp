#include "fs_utils.hpp"

#include <fstream>

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

} // namespace fs_utils
