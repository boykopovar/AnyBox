#include "TextureAllocation.hpp"
#include <anybox/layout/Texture.hpp>
#include <bit>

std::uint32_t AnyBox::Graphics::ComputeTextureSwizzleMode(const D3D12_RESOURCE_DESC& description) {
    return Layout::GetTextureSwizzleMode(std::bit_cast<Layout::TextureDescription>(description));
}

D3D12_RESOURCE_ALLOCATION_INFO AnyBox::Graphics::ComputeTextureAllocation(const D3D12_RESOURCE_DESC& description) {
    auto surface = ComputeTextureSurface(description);
    return {surface.size, surface.alignment};
}

AnyBox::Address::SurfaceLayout AnyBox::Graphics::ComputeTextureSurface(const D3D12_RESOURCE_DESC& description) {
    return Layout::ComputeTextureSurface(std::bit_cast<Layout::TextureDescription>(description));
}
