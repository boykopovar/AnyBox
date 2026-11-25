#pragma once

#include <anybox/platform/Windows.hpp>
#include <anybox/descriptor/Texture.hpp>
#include <d3d12.h>

namespace AnyBox::Graphics {
Descriptor::TextureViewDescription ReadTextureShaderView(const D3D12_RESOURCE_DESC& resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* description, std::uint64_t address);
}
