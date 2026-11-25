#include <anybox/layout/Texture.hpp>
#include <bit>
#include <algorithm>
#include <stdexcept>

AnyBox::Address::SurfaceDescription AnyBox::Layout::DescribeTextureSurface(const TextureDescription& description) {
    if (description.dimension != 3) throw std::runtime_error("Texture allocation dimension is not implemented");
    if (description.width == 0 || description.width > 16384 || description.height == 0 || description.height > 16384 || description.depthOrArraySize == 0 || description.depthOrArraySize > 2048) throw std::invalid_argument("Texture allocation dimensions are invalid");
    if (description.mipLevels == 0) throw std::runtime_error("Implicit texture mip count is not implemented");
    if (description.mipLevels > std::bit_width(std::max(description.width, static_cast<std::uint64_t>(description.height)))) throw std::invalid_argument("Texture mip count exceeds its dimensions");
    if (description.sample.count != 1 || description.sample.quality != 0) throw std::runtime_error("Multisample texture allocation is not implemented");
    auto flags = description.flags;
    if ((flags & ~0x30000Du) != 0 || ((flags & 1) != 0 && (flags & 0x300000) == 0)) throw std::runtime_error("Texture allocation metadata or resource flags are not implemented");
    if (description.alignment != 0 && description.alignment != 4096 && description.alignment != 65536) throw std::runtime_error("Texture allocation alignment is not implemented");
    Address::SurfaceDescription input;
    input.width = static_cast<std::uint32_t>(description.width);
    input.height = description.height;
    input.slices = description.depthOrArraySize;
    input.mipLevels = description.mipLevels;
    input.samples = 1;
    input.fragments = 1;
    input.bitsPerElement = TextureBitsPerPixel(description.format);
    input.swizzleMode = GetTextureSwizzleMode(description);
    input.resourceType = 1;
    input.flags = (flags & 1) | ((flags & 8) == 0 ? 1u << 9 : 0) | ((flags & 4) != 0 ? 1u << 10 : 0) | (1u << 16);
    return input;
}
