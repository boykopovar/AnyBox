#pragma once

#include <anybox/graphics/container/SerializedGraphicsState.hpp>
#include "../../Integer.hpp"
#include <bit>

namespace AnyBox::Graphics::Container::SerializedGraphics {
inline std::uint8_t Byte(std::span<const std::byte> data, std::size_t offset) {
    if (offset >= data.size()) throw std::invalid_argument("Xbox serialized graphics byte is outside its payload");
    return std::to_integer<std::uint8_t>(data[offset]);
}

inline float Float(std::span<const std::byte> data, std::size_t offset) {
    return std::bit_cast<float>(ReadInteger(data, offset));
}

SerializedDepthState ReadDepth(std::span<const std::byte> context);
SerializedRasterizerState ReadRasterizer(std::span<const std::byte> context, std::span<const std::byte> auxiliary);
std::array<SerializedBlendTarget, 8> ReadBlend(std::span<const std::byte> context, std::span<const std::byte> auxiliary);
}
