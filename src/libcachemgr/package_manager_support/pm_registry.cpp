#include "pm_registry.hpp"

#include <type_traits>

#include "cargo/cargo.hpp"
#include "composer/composer.hpp"
#include "go/go.hpp"
#include "npm/npm.hpp"
#include "pub/pub.hpp"

using namespace libcachemgr::package_manager_support;

namespace {

static pm_registry::pm_registry_t _pm_registry{};
static pm_registry::pm_user_registry_t _pm_user_registry{};

/// concept which checks if the type is a base of pm_base
template<typename T>
concept is_package_manager = std::is_base_of<pm_base, T>::value;

template<is_package_manager T>
inline constexpr void register_package_manager()
{
    // need to make a temporary instance to receive the pm_name()
    _pm_registry[T().pm_name()] = std::make_unique<T>();
}

/// register all package managers before main()
static const bool _pm_registry_init = [](){
    register_package_manager<cargo>();
    register_package_manager<composer>();
    register_package_manager<go>();
    register_package_manager<npm>();
    register_package_manager<pub>();

    return true;
}();

} // anonymous namespace

const pm_registry::pm_registry_t &pm_registry::registry() noexcept
{
    return _pm_registry;
}

const pm_base *const pm_registry::find_package_manager(std::string_view name) noexcept
{
    if (const auto &pm = _pm_registry.find(name); pm != _pm_registry.end())
    {
        return pm->second.get();
    }
    else
    {
        return nullptr;
    }
}

void pm_registry::register_user_package_manager(const pm_base *const pm) noexcept
{
    _pm_user_registry.emplace(pm->pm_name(), pm);
}

const pm_registry::pm_user_registry_t &pm_registry::user_registry() noexcept
{
    return _pm_user_registry;
}
