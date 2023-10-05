#include "libcachemgr.hpp"

using namespace libcachemgr;

std::string_view program_metadata::application_name = "cachemgr";
std::string_view program_metadata::application_version = "0.0.0-dev";
std::string_view program_metadata::platform_name = PROJECT_PLATFORM_NAME;

std::string_view program_metadata::git_branch{};
std::string_view program_metadata::git_commit{};
bool program_metadata::git_is_dirty{};
