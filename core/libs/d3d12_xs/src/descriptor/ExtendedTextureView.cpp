#include "ExtendedTextureView.hpp"
#include "TextureShaderDescription.hpp"
#include "../resource/Texture.hpp"
#include <anybox/layout/Texture.hpp>
#include <algorithm>
#include <bit>

AnyBox::Descriptor::ImageWords AnyBox::Graphics::EncodeTextureShaderViewX(const D3D12_RESOURCE_DESC& resource, const XboxShaderViewDescription& description) {
    if (resource.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D) throw std::runtime_error("Xbox extended texture shader view dimension is not implemented");
    if (description.address == UINT64_MAX || std::ranges::any_of(std::span(description.parameters).first(3), [](auto value) { return value != 0; })) throw std::runtime_error("Xbox extended texture view auxiliary addresses are not implemented");
    if ((description.parameters[3] & ~3u) != 0) throw std::runtime_error("Xbox extended texture view control flags are not implemented");
    if ((static_cast<std::uint32_t>(resource.Flags) & 0x300000u) != 0x300000u) throw std::runtime_error("Xbox extended texture view compression metadata is not implemented");
    auto surface = Layout::DescribeTextureSurface(std::bit_cast<Layout::TextureDescription>(resource));
    auto view = ReadTextureShaderView(resource, &description.view, description.address);
    auto format = TextureFormat(resource.Format);
    Descriptor::Texture2DDescription texture{surface.width, surface.height, surface.slices, surface.mipLevels, format.hardwareFormat, surface.swizzleMode};
    return Descriptor::EncodeTexture2D(texture, view);
}
