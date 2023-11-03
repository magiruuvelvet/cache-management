#pragma once

#include "models.hpp"

#include <fmt/core.h>

template<> struct fmt::formatter<libcachemgr::database::cache_trend> : formatter<string_view> {
    auto format(const libcachemgr::database::cache_trend &cache_trend, format_context &ctx) const {
        const auto fmt = fmt::format("cache_trend({}={}, {}={}, {}={}, {}={})",
            cache_trend.timestamp.name, cache_trend.timestamp.value,
            cache_trend.cache_mapping_id.name, cache_trend.cache_mapping_id.value,
            cache_trend.package_manager.name, cache_trend.package_manager.value,
            cache_trend.cache_size.name, cache_trend.cache_size.value);
        return formatter<string_view>::format(fmt, ctx);
    }
};
