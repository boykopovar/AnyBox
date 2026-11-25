#pragma once

#include <anybox/graphics/container/NativeBindings.hpp>
#include <anybox/graphics/container/RootSignature.hpp>
#include <optional>

namespace AnyBox::Shader {
void ValidateNativeComputeRootLayout(const Graphics::Container::RootSignatureLayout& layout);
void ValidateNativeRootBinding(const Graphics::Container::RootSignatureLayout& layout, const Graphics::Container::NativeRootBinding& binding, std::optional<Graphics::Container::RootRangeKind> rangeKind = std::nullopt);
}
