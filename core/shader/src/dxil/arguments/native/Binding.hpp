#pragma once

#include <anybox/graphics/container/NativeBindings.hpp>
#include <anybox/graphics/container/RootSignature.hpp>

namespace AnyBox::Shader {
void ValidateNativeRootBinding(const Graphics::Container::RootSignatureLayout& layout, const Graphics::Container::NativeRootBinding& binding);
}
