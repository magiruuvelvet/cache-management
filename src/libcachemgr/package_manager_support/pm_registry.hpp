#pragma once

#include "pm_base.hpp"

#include <memory>
#include <unordered_map>

namespace libcachemgr {
namespace package_manager_support {

struct pm_registry final
{
public:
    using pm_registry_t = std::unordered_map<pm_base::pm_name_type, std::unique_ptr<pm_base>>;

    /// get the package manager registry
    static const pm_registry_t &registry() noexcept;

    /// finds the package manager with the given name or returns nullptr
    static const pm_base *const find_package_manager(std::string_view name) noexcept;
};

} // namespace package_manager_support
} // namespace libcachemgr
