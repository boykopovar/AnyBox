#include <anybox/address/Context.hpp>
#include <array>
#include <iostream>
#include <stdexcept>

void CheckUnsupportedCopyProfile(const AnyBox::Address::Context& context);

namespace {
void CheckMipChain(const AnyBox::Address::Context& context, AnyBox::Address::SurfaceDescription description) {
    description.width = 1024;
    description.height = 1024;
    description.mipLevels = 11;
    std::array<AnyBox::Address::MipLayout, 11> mips{};
    auto layout = context.Compute(description, mips);
    if (layout.size < 1024 * 1024 * 4 || layout.firstMipInTail >= mips.size()) throw std::runtime_error("Mip chain allocation or tail is invalid");

    for (std::size_t index = 0; index < mips.size(); ++index) {
        auto dimension = 1024U >> index;
        const auto& mip = mips[index];
        if (mip.pixelPitch < dimension || mip.pixelHeight < dimension || mip.blockOffset >= layout.size) throw std::runtime_error("Mip bounds do not contain their image");
    }

    description.width = 0;
    try {
        context.Compute(description);
    } catch (const std::invalid_argument&) {
        return;
    }
    throw std::runtime_error("Invalid surface dimensions were accepted");
}
}

int main() {
    try {
        AnyBox::Address::Context context({13, 0x8F, 0x28, 0});
        AnyBox::Address::SurfaceDescription description;
        description.width = 3840;
        description.height = 2160;
        description.slices = 1;
        description.mipLevels = 1;
        description.samples = 1;
        description.fragments = 1;
        description.bitsPerElement = 32;
        description.swizzleMode = 9;
        description.resourceType = 1;
        description.flags = 1 | (1U << 9) | (1U << 10);
        std::array<AnyBox::Address::MipLayout, 1> mips{};
        auto layout = context.Compute(description, mips);
        std::cout << layout.size << ' ' << layout.alignment << ' ' << layout.pixelPitch << ' ' << layout.pixelHeight << '\n';
        if (layout.size != 33423360 || layout.alignment != 65536 || layout.pixelPitch != 3840 || layout.pixelHeight != 2176) throw std::runtime_error("GFX10 surface does not match its expected block geometry");
        if (mips[0].offset != 0 || mips[0].pixelPitch != layout.pixelPitch || mips[0].pixelHeight != layout.pixelHeight) throw std::runtime_error("GFX10 mip layout does not match its allocation");
        CheckMipChain(context, description);
        CheckUnsupportedCopyProfile(context);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
