#include "Computer.hpp"
#include <stdexcept>

std::int32_t AnyBox::XG::TextureComputer::CopyIntoSubresource(IXGTextureComputer* self, void* tiled, std::uint32_t plane, std::uint32_t subresource, const void* linear, std::uint32_t rowPitch, std::uint32_t slicePitch) {
    const auto& object = from(self);
    if (plane >= object.layout.planeCount || subresource >= object.description.depthOrArraySize * object.layout.mipLevels) throw std::out_of_range("Texture copy plane or subresource is invalid");
    Address::SurfaceCopy copy{subresource % object.layout.mipLevels, subresource / object.layout.mipLevels, rowPitch, slicePitch};
    Layout::GetTextureAddressContext().CopyToSurface(object.addressing, tiled, linear, copy);
    return 0;
}

std::int32_t AnyBox::XG::TextureComputer::CopyFromSubresource(IXGTextureComputer* self, void* linear, std::uint32_t plane, std::uint32_t subresource, std::uint32_t sample, const void* tiled, std::uint32_t rowPitch, std::uint32_t slicePitch) {
    const auto& object = from(self);
    if (plane >= object.layout.planeCount || subresource >= object.description.depthOrArraySize * object.layout.mipLevels || sample != 0) throw std::out_of_range("Texture copy plane, subresource or sample is invalid");
    Address::SurfaceCopy copy{subresource % object.layout.mipLevels, subresource / object.layout.mipLevels, rowPitch, slicePitch};
    Layout::GetTextureAddressContext().CopyFromSurface(object.addressing, linear, tiled, copy);
    return 0;
}
