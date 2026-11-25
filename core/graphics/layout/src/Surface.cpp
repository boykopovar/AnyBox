#include <anybox/layout/Texture.hpp>

AnyBox::Address::SurfaceLayout AnyBox::Layout::ComputeTextureSurface(const TextureDescription& description, std::span<Address::MipLayout> mips) {
    auto input = DescribeTextureSurface(description);
    static const Address::Context context({13, 0x8F, 0x28, 0});
    return context.Compute(input, mips);
}
