#pragma once

#include <type_traits>
#include <cstdint>
#include <string>

namespace number_utils {

template<typename IntegralType>
concept is_integral_type = requires {
    requires std::is_integral_v<IntegralType>;
};

/**
 * Parses the given string into a number without throwing an exception.
 *
 * To check for parsing errors, pass a bool pointer to the @p ok parameter.
 *
 * @tparam IntegralType any integral type
 * @param str the string to parse
 * @param ok optional parsing error indicator
 * @return the parsed number, or 0 if parsing failed
 */
template<is_integral_type IntegralType>
IntegralType parse_integer(const std::string &str, bool *ok = nullptr) noexcept
{
    struct UnsupportedIntegralType;

    IntegralType result{0};
    bool is_ok{};

    if constexpr (std::is_same_v<IntegralType, std::uint32_t>)
    {
        char *end_ptr{};
        result = std::strtoul(str.c_str(), &end_ptr, 10);
        is_ok = *end_ptr == 0;
    }
    else
    {
        static_assert(std::is_same_v<IntegralType, UnsupportedIntegralType>,
            "parse_integer: unsupported integral type");
    }

    if (!is_ok) {
        result = 0;
    }
    if (ok) {
        *ok = is_ok;
    }
    return result;
}

} // namespace number_utils
