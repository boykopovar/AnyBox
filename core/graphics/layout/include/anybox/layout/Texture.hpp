#pragma once

#include "Description.hpp"
#include <anybox/address/Context.hpp>

namespace AnyBox::Layout {
std::uint32_t TextureBitsPerPixel(std::uint32_t format);
std::uint32_t GetTextureSwizzleMode(const TextureDescription& description);
std::uint32_t SuggestTextureLayout(const TextureDescription& description);
Address::SurfaceDescription DescribeTextureSurface(const TextureDescription& description);
Address::SurfaceLayout ComputeTextureSurface(const TextureDescription& description, std::span<Address::MipLayout> mips = {});
}
