#pragma once

#include "NativeCompute.hpp"
#include "RootArguments.hpp"
#include <anybox/graphics/container/NativeBindings.hpp>

namespace AnyBox::Shader {
std::vector<BufferBinding> ReadNativeDirectBufferBindings(const Graphics::Container::NativeBindings& bindings, const Graphics::Container::RootSignatureLayout& root, std::uint32_t visibility);
void BindNativeRootArguments(NativeComputeTranslation& translation, std::span<const Graphics::Container::NativeRootBinding> roots, const RootArgumentPlan& plan);
void BindNativeGraphicsRootArguments(ShaderRecompiler::IrProgram& program, std::span<const Graphics::Container::NativeRootBinding> roots, const RootArgumentPlan& plan);
}
