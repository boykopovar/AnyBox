#pragma once

#include <anybox/graphics/container/NativeBindings.hpp>
#include <anybox/graphics/container/RootSignature.hpp>
#include <optional>
#include <anybox/shader/NativeRootArgumentWriter.hpp>

namespace AnyBox::Shader {
void ValidateNativeRootLayout(const Graphics::Container::RootSignatureLayout& layout, std::uint32_t visibility);
void ValidateNativeRootBinding(const Graphics::Container::RootSignatureLayout& layout, const Graphics::Container::NativeRootBinding& binding, std::optional<Graphics::Container::RootRangeKind> rangeKind = std::nullopt, NativeRootBindingMode mode = NativeRootBindingMode::LogicalRegisters, std::uint32_t visibility = 0);
}
