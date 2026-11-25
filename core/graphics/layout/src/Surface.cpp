#include <anybox/layout/Texture.hpp>

const AnyBox::Address::Context& AnyBox::Layout::GetTextureAddressContext() {
    static const Address::Context context(Address::SeriesXHardware);
    return context;
}

AnyBox::Address::SurfaceLayout AnyBox::Layout::ComputeTextureSurface(const TextureDescription& description, std::span<Address::MipLayout> mips) {
    return GetTextureAddressContext().Compute(DescribeTextureSurface(description), mips);
}
