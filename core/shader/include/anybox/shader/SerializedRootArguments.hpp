#pragma once

#include <anybox/graphics/container/NativeBindings.hpp>
#include <anybox/graphics/container/RootSignature.hpp>

namespace AnyBox::Shader {
std::vector<Graphics::Container::NativeRootBinding> BuildSerializedComputeRoots(std::span<const Graphics::Container::NativeResourceBinding> resources, const Graphics::Container::RootSignatureLayout& layout);
}
