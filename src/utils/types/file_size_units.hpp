#pragma once

#include <ostream>
//#include <iomanip>
#include <cinttypes>
#include <cmath>

namespace file_size_units {

enum class file_size_unit : unsigned
{
    kibi = 1024,
    kilo = 1000,
};

/**
 * `std::ostream` helper to print a file size in a human-readable format.
 *
 * The number of decimal places and the unit are configurable.
 *
 * Defaults to 2 decimal places and `kilo` (1000) units.
 *
 *
 * Support for {fmt} can be enabled with this code snippet:
 * ```cpp
 * #include <fmt/ostream.h>
 * template<> struct fmt::formatter<human_readable_file_size> : ostream_formatter{};
 * ```
 */
struct human_readable_file_size final
{
    using size_type = std::uintmax_t;
    using decimal_type = unsigned;

    explicit human_readable_file_size(size_type size)
        : size(size), decimal_places(2), unit(double(file_size_unit::kilo))
    {}

    explicit human_readable_file_size(size_type size, decimal_type decimal_places)
        : size(size), decimal_places(decimal_places), unit(double(file_size_unit::kilo))
    {}

    explicit human_readable_file_size(size_type size, decimal_type decimal_places, file_size_unit unit)
        : size(size), decimal_places(decimal_places), unit(double(unit))
    {}

private:
    const size_type size;
    const decimal_type decimal_places;
    const double unit;

    friend std::ostream &operator<<(std::ostream &os, const human_readable_file_size &hr)
    {
        int o{};
        double mantissa = hr.size;
        for (; mantissa >= hr.unit; mantissa /= hr.unit, ++o);
        const auto pow = std::pow(10, hr.decimal_places);
        const double rounded = std::round(mantissa * pow) / pow;
        os << rounded << "BKMGTPE"[o];
        //os << std::fixed << std::setprecision(hr.decimal_places) << std::setfill('0') << rounded << "BKMGTPE"[o];
        return o ? os << 'B' : os;
    }
};

} // namespace file_size_units
