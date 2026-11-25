#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace AnyBox::Graphics::Container {
inline std::size_t FspxPartIndex(std::uint32_t kind) {
    switch (kind) {
    case 0: return 0;
    case 1: return 1;
    case 6: return 2;
    case 7: return 3;
    default: throw std::runtime_error("Xbox serialized pipeline part kind is not implemented");
    }
}
}
