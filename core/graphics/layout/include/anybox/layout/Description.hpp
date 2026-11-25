#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace AnyBox::Layout {
struct SampleDescription {
    std::uint32_t count;
    std::uint32_t quality;
};

struct TextureDescription {
    std::uint32_t dimension;
    std::uint64_t alignment;
    std::uint64_t width;
    std::uint32_t height;
    std::uint16_t depthOrArraySize;
    std::uint16_t mipLevels;
    std::uint32_t format;
    SampleDescription sample;
    std::uint32_t layout;
    std::uint32_t flags;
};
static_assert(std::is_standard_layout_v<TextureDescription> && std::is_trivially_copyable_v<TextureDescription>);
static_assert(sizeof(TextureDescription) == 56);
static_assert(offsetof(TextureDescription, width) == 16);
static_assert(offsetof(TextureDescription, depthOrArraySize) == 28);
static_assert(offsetof(TextureDescription, layout) == 44);
static_assert(offsetof(TextureDescription, flags) == 48);
}
