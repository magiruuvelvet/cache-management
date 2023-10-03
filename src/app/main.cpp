#include <cstdio>

#include <utils/logging_helper.hpp>
#include <utils/os_utils.hpp>
#include <utils/xdg_paths.hpp>

#include <libcachemgr/logging.hpp>
#include <libcachemgr/config.hpp>
#include <libcachemgr/cachemgr.hpp>

namespace {
class basic_utils_logger : public logging_helper
{
public:
    void log_info(const std::string &message) override {
        std::fprintf(stderr, "[inf] %s\n", message.c_str()); }
    void log_warning(const std::string &message) override {
        std::fprintf(stderr, "[wrn] %s\n", message.c_str()); }
    void log_error(const std::string &message) override {
        std::fprintf(stderr, "[err] %s\n", message.c_str()); }
};
} // anonymous namespace

int main(int argc, char **argv)
{
    // catch errors early during first os_utils function calls
    logging_helper::set_logger(std::make_shared<basic_utils_logger>());

    // receive essential directories
    const auto home_dir = os_utils::get_home_directory();
    const auto xdg_cache_home = xdg_paths::get_xdg_cache_home();

    // initialize logging subsystem (includes os_utils function calls)
    libcachemgr::init_logging(libcachemgr::logging_config{
        .log_file_path = xdg_cache_home + "/cachemgr.log",
    });

    cachemgr_t cachemgr;

    std::printf("HOME: %s\n", home_dir.c_str());
    std::printf("XDG_CACHE_HOME: %s\n", xdg_cache_home.c_str());

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
