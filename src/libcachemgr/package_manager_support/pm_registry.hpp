#pragma once

#include "pm_base.hpp"

#include <memory>
#include <unordered_map>

namespace libcachemgr {
namespace package_manager_support {

// NOTE: this struct is not thread-safe.
struct pm_registry final
{
public:
    using pm_registry_t = std::unordered_map<pm_base::pm_name_type, std::unique_ptr<pm_base>>;
    using pm_user_registry_t = std::unordered_map<pm_base::pm_name_type, const pm_base *const>;

    /// get the package manager registry
    static const pm_registry_t &registry() noexcept;

    /// finds the package manager with the given name or returns nullptr
    static const pm_base *const find_package_manager(std::string_view name) noexcept;

    /// adds the given package manager to the user registry
    static void register_user_package_manager(const pm_base *const pm) noexcept;

    /// get the user package manager registry
    static const pm_user_registry_t &user_registry() noexcept;

private:
    // disable construct, copy and move
    pm_registry() = delete;
    pm_registry(const pm_registry &) = delete;
    pm_registry(pm_registry &&) = delete;
    pm_registry &operator=(const pm_registry &) = delete;
    pm_registry &operator=(pm_registry &&) = delete;
    ~pm_registry() = delete;
};

} // namespace package_manager_support
} // namespace libcachemgr
