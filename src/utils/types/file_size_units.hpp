#pragma once

#include <ostream>
#include <cinttypes>

enum file_size_unit : int
{
    kibi = 1024,
    kilo = 1000,
};

struct human_readable_file_size final
{
    explicit human_readable_file_size(const std::uintmax_t &size)
        : size(size) {}

private:
    std::uintmax_t size;

    friend std::ostream &operator<<(std::ostream &os, const human_readable_file_size &hr)
    {
        int o{};
        double mantissa = hr.size;
        for (; mantissa >= double{kilo}; mantissa /= double{kilo}, ++o);
        os << std::ceil(mantissa * 10.) / 10. << "BKMGTPE"[o];
        // return o ? os << "B (" << hr.size << " bytes)" : os;
        return o ? os << 'B' : os;
    }
};
