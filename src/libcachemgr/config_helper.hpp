#pragma once

#include "config.hpp"
#include "logging.hpp"

#include <string_view>

#include <fmt/core.h>

#include <ryml.hpp>

namespace libcachemgr {
namespace detail {

namespace {
    /// collection of yaml data types for generic validation
    enum key_type
    {
        sequence,
        map,
        string,
    };
} // anonymous namespace

template<typename ErrorCollectionType>
inline auto has_any_errors(const ErrorCollectionType &error_collection) -> bool
{
    return std::any_of(error_collection.cbegin(), error_collection.cend(), [](bool success){
        return !success;
    });
};

/// validate the datatype of the key
template<typename NodeType>
inline auto validate_expected_type(const NodeType &node_ref, const char *key, key_type expected_type) -> bool
{
    switch (expected_type)
    {
        case sequence: return node_ref[key].is_seq(); break;
        case map:      return node_ref[key].is_map(); break;
        case string:   return node_ref[key].is_keyval(); break;
        default:       return false;
    }
};

} // namespace detail
} // namespace libcachemgr

template<> struct fmt::formatter<libcachemgr::detail::key_type> : formatter<string_view> {
    auto format(libcachemgr::detail::key_type key_type, format_context &ctx) const {
        using namespace libcachemgr::detail;
        string_view           name = "key_type::(unknown)";
        switch (key_type) {
            case sequence:    name = "key_type::sequence"; break;
            case map:         name = "key_type::map"; break;
            case string:      name = "key_type::string"; break;
        }
        return formatter<string_view>::format(name, ctx);
    }
};
