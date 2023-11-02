#pragma once

#include <string>
#include <optional>
#include <cstdint>
#include <type_traits>

namespace libcachemgr {
namespace database {

/**
 * Compile-time attribute name as template parameter.
 */
template<std::size_t N>
struct AttributeName
{
    constexpr AttributeName(const char (&str)[N]) {
        std::copy_n(str, N, value);
    }

    char value[N];
};

/**
 * Represents a database column name with its value.
 *
 * @tparam FieldName the column name which must be a compile-time constant.
 * @tparam FieldType the type of the column value.
 */
template<AttributeName FieldName, typename FieldType>
struct field_pair final
{
    inline constexpr field_pair() = default;

    /// default field value constructor
    inline constexpr field_pair(const FieldType &value) {
        this->value = value;
    }

    /// allow compile-time strings to be used as a field value
    template<size_t N>
    inline constexpr field_pair(char const (&value)[N]) {
        this->value = value;
    }

    /// allow std::nullopt_t to be used as a field value
    template<typename = std::enable_if<std::is_same_v<std::nullopt_t, FieldType>>>
    inline constexpr field_pair(const auto &value) {
        this->value = value;
    }

    /// default copy assignment operator
    inline constexpr FieldType &operator=(const FieldType &value) {
        this->value = value;
        return this;
    }

    /// implicit conversion to FieldType
    inline constexpr operator const FieldType &() const {
        return this->value;
    }

    /// implicit conversion to FieldType
    inline constexpr operator FieldType &() {
        return this->value;
    }

    /// value type alias (required for constexpr operations)
    using value_type = FieldType;

    /// read-only view of the column name
    const std::string_view name = FieldName.value;

    /// column value
    FieldType value;
};

/**
 * cache trend record
 */
struct cache_trend final
{
    /// UTC unix timestamp when the trend was calculated
    field_pair<"timestamp", std::uint64_t> timestamp;

    /// user-defined cache_mappings[].id
    field_pair<"cache_mapping_id", std::string> cache_mapping_id;

    /// name of the package manager
    field_pair<"package_manager", std::optional<std::string>> package_manager;

    /// cache size in bytes
    field_pair<"cache_size", std::uintmax_t> cache_size;
};

} // namespace database
} // namespace libcachemgr
