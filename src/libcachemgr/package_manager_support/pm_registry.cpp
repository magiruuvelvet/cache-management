#include "pm_registry.hpp"

//#include <type_traits>

#include "go/go.hpp"
#include "npm/npm.hpp"

using namespace libcachemgr::package_manager_support;

namespace {

static pm_registry::pm_registry_t _pm_registry{};

} // anonymous namespace

/// concept which checks if the type is a base of pm_base
template<typename T>
concept pm_base_concept = std::is_base_of<pm_base, T>::value;

/** just a failed experiment */
// /// register multiple package managers using a variadic template
// template<pm_base_concept pm_type, typename... Args>
// constexpr void register_package_managers()
// {
//     (_pm_registry[pm_type::pm_name_static()] = std::make_unique<pm_type>());
// }

template<pm_base_concept T>
inline constexpr void register_package_manager()
{
    _pm_registry[T::pm_name_static()] = std::make_unique<T>();
}

void pm_registry::populate_registry()
{
    // register_package_managers<
    //     go,
    //     npm
    // >();

    register_package_manager<go>();
    register_package_manager<npm>();
}

const pm_registry::pm_registry_t &pm_registry::registry() noexcept
{
    return _pm_registry;
}
