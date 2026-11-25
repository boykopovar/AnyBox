#pragma once

#include "Descriptions.hpp"
#include <anybox/descriptor/Texture.hpp>

namespace AnyBox::Graphics {
Descriptor::ImageWords EncodeTextureShaderViewX(const D3D12_RESOURCE_DESC& resource, const XboxShaderViewDescription& description);
}
