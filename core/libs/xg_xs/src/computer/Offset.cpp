#include "Computer.hpp"
#include <stdexcept>

std::uint64_t AnyBox::XG::TextureComputer::GetTexelOffset(IXGTextureComputer* self, std::uint32_t plane, std::uint32_t mip, std::uint64_t x, std::uint32_t y, std::uint32_t z, std::uint32_t sample, std::uint32_t* bitPosition) {
    const auto& object = from(self);
    if (plane >= object.layout.planeCount || x > UINT32_MAX) throw std::out_of_range("Texture address plane or horizontal coordinate is invalid");
    auto offset = Layout::GetTextureAddressContext().GetOffset(object.addressing, {static_cast<std::uint32_t>(x), y, z, mip, sample});
    if (offset.address >= object.surface.size) throw std::runtime_error("Texture address exceeds its allocation");
    if (bitPosition) *bitPosition = offset.bitPosition;
    return offset.address;
}
