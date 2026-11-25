#include "Computer.hpp"
#include <stdexcept>

AnyBox::XG::TextureComputer::TextureComputer(const XGTextureDescription& description) : description(description) {
    vtable = &table;
    if (description.mipLevels != 1) throw std::runtime_error("Texture computer mip chain layout is not implemented");
    addressing = Layout::DescribeTextureSurface(description);
    surface = Layout::GetTextureAddressContext().Compute(addressing);
    if (addressing.swizzleMode != 0 && (surface.firstMipInTail == 0 || surface.mipChainInTail)) throw std::runtime_error("Texture computer packed mip layout is not implemented");
    layout.size = surface.size;
    layout.alignment = surface.alignment;
    layout.mipLevels = 1;
    layout.planeCount = 1;
    layout.dimension = description.dimension;
    auto& plane = layout.planes[0];
    plane.usage = 1;
    plane.size = surface.size;
    plane.sliceSize = surface.sliceSize;
    plane.alignment = surface.alignment;
    plane.elementBytes = surface.bitsPerElement / 8;
    plane.standardMips = 1;
    auto& mip = plane.mips[0];
    mip.size = mip.sliceSize = surface.sliceSize;
    mip.pitchPixels = surface.pixelPitch;
    mip.pitchBytes = surface.pitch * plane.elementBytes;
    mip.alignment = surface.alignment;
    mip.padded = {surface.pitch, surface.height, description.depthOrArraySize};
    mip.elements = mip.texels = {static_cast<std::uint32_t>(description.width), description.height};
    mip.depthOrArraySize = description.depthOrArraySize;
    mip.samples = description.sample.count;
    mip.swizzleMode = Layout::GetTextureSwizzleMode(description);
    mip.sliceDepth = surface.blockSlices;
}

std::int32_t XGCreateTextureComputer(const XGTextureDescription* description, IXGTextureComputer** computer) noexcept(false) {
    if (!computer) throw std::invalid_argument("Texture computer output is null");
    *computer = nullptr;
    if (!description) throw std::invalid_argument("Texture computer description is null");
    *computer = new AnyBox::XG::TextureComputer(*description);
    return 0;
}
