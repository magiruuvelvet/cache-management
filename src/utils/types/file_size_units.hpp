#pragma once

#include <ostream>
#include <iomanip>
#include <cinttypes>
#include <cmath>

namespace file_size_units {

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

    enum class file_size_unit : size_type
    {
        kibi = 1024,
        kilo = 1000,
    };

    explicit human_readable_file_size(size_type size)
        : size(size), decimal_places(2), unit(file_size_unit::kilo), unit_d(double(file_size_unit::kilo))
    {}

    explicit human_readable_file_size(size_type size, decimal_type decimal_places)
        : size(size), decimal_places(decimal_places), unit(file_size_unit::kilo), unit_d(double(file_size_unit::kilo))
    {}

    explicit human_readable_file_size(size_type size, decimal_type decimal_places, file_size_unit unit)
        : size(size), decimal_places(decimal_places), unit(unit), unit_d(double(unit))
    {}

private:
    const size_type size;
    const decimal_type decimal_places;
    const file_size_unit unit;
    const double unit_d;

    static constexpr const auto steps = "BKMGTPE";

    friend std::ostream &operator<<(std::ostream &os, const human_readable_file_size &hr)
    {
        int step{};
        double mantissa = hr.size;
        for (; mantissa >= hr.unit_d; mantissa /= hr.unit_d, ++step);
        const auto pow = std::pow(10, hr.decimal_places);
        const double rounded = std::round(mantissa * pow) / pow;
        if (hr.size < size_type(hr.unit)) {
            os << rounded << steps[step];
        } else {
            os << std::fixed << std::setprecision(hr.decimal_places) << std::setfill('0') << rounded << steps[step];
        }
        return step ? os << steps[0] : os; // ensure this is always 'B' in case you refactor {steps}
    }
};

} // namespace file_size_units
