#include "Computer.hpp"
#include <stdexcept>

std::int32_t AnyBox::XG::TextureComputer::GetResourceLayout(IXGTextureComputer* self, XGTextureLayout* layout) {
    if (!layout) throw std::invalid_argument("Texture computer layout output is null");
    *layout = from(self).layout;
    return 0;
}

std::uint64_t AnyBox::XG::TextureComputer::GetSize(IXGTextureComputer* self) { return from(self).layout.size; }
std::uint64_t AnyBox::XG::TextureComputer::GetAlignment(IXGTextureComputer* self) { return from(self).layout.alignment; }

std::uint64_t AnyBox::XG::TextureComputer::GetMipOffset(IXGTextureComputer* self, std::uint32_t plane, std::uint32_t mip) {
    const auto& layout = from(self).layout;
    if (plane >= layout.planeCount || mip >= layout.mipLevels) throw std::out_of_range("Texture computer mip or plane index is invalid");
    return layout.planes[plane].offset + layout.planes[plane].mips[mip].offset;
}

bool AnyBox::XG::TextureComputer::IsTiledResource(IXGTextureComputer* self) { (void)from(self); return false; }
