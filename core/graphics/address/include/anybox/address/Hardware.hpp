#pragma once

#include <cstdint>

namespace AnyBox::Address {
struct HardwareConfiguration {
    std::uint32_t engine;
    std::uint32_t family;
    std::uint32_t revision;
    std::uint32_t addressConfiguration;
    bool operator==(const HardwareConfiguration&) const = default;
};
inline constexpr HardwareConfiguration SeriesXHardware{13, 0x8F, 0x28, 0x345};
}
