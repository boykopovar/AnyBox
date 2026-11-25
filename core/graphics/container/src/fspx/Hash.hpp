#pragma once

#include <array>
#include <cstddef>
#include <span>

namespace AnyBox::Graphics::Container {
std::array<std::byte, 16> ReadSerializedCodeHash(std::span<const std::byte> code);
}
