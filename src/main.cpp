#include <cstdio>

#include <utils/logging_helper.hpp>
#include <utils/os_utils.hpp>

#include "config.hpp"
#include "cachemgr.hpp"

// TODO: just for testing, will be replaced with an external logging library
class logger : public logging_helper
{
public:
    void log_info(const std::string &message) override {
        std::fprintf(stderr, "[inf] %s\n", message.c_str()); }
    void log_warning(const std::string &message) override {
        std::fprintf(stderr, "[wrn] %s\n", message.c_str()); }
    void log_error(const std::string &message) override {
        std::fprintf(stderr, "[err] %s\n", message.c_str()); }
};

int main(int argc, char **argv)
{
    // obtain log messages produced by library functions
    logging_helper::set_logger(std::make_shared<logger>());

    cachemgr_t cachemgr;

    // receive essential directories
    const auto home_dir = os_utils::get_home_directory();
    const auto xdg_cache_home = os_utils::getenv("XDG_CACHE_HOME", home_dir + "/.cache");

    std::printf("HOME: %s\n", home_dir.c_str());
    std::printf("XDG_CACHE_HOME: %s\n", xdg_cache_home.c_str());

    // scan HOME and XDG_CACHE_HOME for symlinked cache directories
    for (auto&& dir : {home_dir, xdg_cache_home})
    {
        const bool result = cachemgr.find_symlinked_cache_directories(dir, "/caches/1000");

        std::printf("result = %s\n", result ? "true" : "false");

        for (auto&& dir : cachemgr.symlinked_cache_directories())
        {
            std::printf("%s -> %s\n", dir.original_path.c_str(), dir.symlinked_path.c_str());
        }
    }

    std::printf("is_mount_point(/home): %i\n", os_utils::is_mount_point("/home"));
    std::printf("is_mount_point(/caches/1000): %i\n", os_utils::is_mount_point("/caches/1000"));

    configuration_t::file_error file_error;
    configuration_t::parse_error parse_error;
    configuration_t config("./test.yaml", &file_error, &parse_error);

    std::printf("file_error = %i, parse_error = %i\n", file_error, parse_error);

    return 0;
}
