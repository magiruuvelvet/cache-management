#include "pm_registry.hpp"

#include <type_traits>

#include "composer/composer.hpp"
#include "go/go.hpp"
#include "npm/npm.hpp"
#include "pub/pub.hpp"

using namespace libcachemgr::package_manager_support;

namespace {

static pm_registry::pm_registry_t _pm_registry{};

/// concept which checks if the type is a base of pm_base
template<typename T>
concept pm_base_concept = std::is_base_of<pm_base, T>::value;

template<pm_base_concept T>
inline constexpr void register_package_manager()
{
    // need to make a temporary instance to receive the pm_name()
    _pm_registry[T().pm_name()] = std::make_unique<T>();
}

/// register all package managers before main()
static const bool _pm_registry_init = [](){
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