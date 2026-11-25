#include <anybox/layout/Texture.hpp>
#include <stdexcept>

std::uint32_t AnyBox::Layout::GetTextureSwizzleMode(const TextureDescription& description) {
    auto bits = TextureBitsPerPixel(description.format);
    if (description.layout == 1) return 0;
    if (description.layout >= 256 && description.layout <= 287) {
        auto mode = description.layout - 256;
        if (mode == 0 || mode == 5 || mode == 21 || mode == 27) return mode;
    }
    if (description.layout != 0) throw std::runtime_error("Texture allocation layout is not implemented");
    if ((description.flags & 1) != 0) return 27;
    if ((description.flags & 4) == 0) return 5;
    if (description.width > 16384 || description.height > 16384) throw std::invalid_argument("Texture swizzle dimensions exceed their limits");
    return description.width * description.height * bits <= 0x80000 ? 21 : 27;
}

std::uint32_t AnyBox::Layout::SuggestTextureLayout(const TextureDescription& description) {
    auto automatic = description;
    automatic.layout = 0;
    return 0x100u | DescribeTextureSurface(automatic).swizzleMode;
}
