#include "fs_utils.hpp"

#include <fstream>

#include "logging_helper.hpp"

namespace fs_utils {

std::string read_text_file(std::string_view path) noexcept
{
    std::string buffer;

    std::ifstream fstream(path, std::ios::in);
    if (!fstream)
    {
        logging_helper::get_logger()->log_error("failed to open file: " + std::string{path});
        return {};
    }

    fstream.seekg(0, std::ios::end);
    buffer.resize(fstream.tellg()); // preallocate buffer
    fstream.seekg(0);
    fstream.read(buffer.data(), buffer.size());
    fstream.close();

    return buffer;
}

} // namespace fs_utils
