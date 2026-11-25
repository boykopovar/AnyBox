#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace AnyBox::Graphics::Container {
struct SerializedPixelInterface {
    std::uint32_t waveSize;
    std::uint32_t inputAddress;
    std::uint32_t inputCount;
    std::uint32_t depthControl;
    std::uint32_t barycentricControl;
    std::uint32_t colorFormat;
    std::array<std::uint32_t, 32> interpolators;
};

SerializedPixelInterface ReadSerializedPixelInterface(std::span<const std::byte> state);
}
