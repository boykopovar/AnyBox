#pragma once

#include "NativeRootArgumentWriter.hpp"
#include "ResourcePlan.hpp"
#include <Optimization/SrtWalker.hpp>

namespace AnyBox::Shader {
class NativeComputeArguments {
public:
    NativeComputeArguments(const ShaderRecompiler::IrResourcePlan& plan, const Graphics::Container::RootSignatureLayout& layout, std::span<const Graphics::Container::NativeRootBinding> roots, std::span<const std::uint32_t> shaderRegisters, NativeRootBindingMode mode = NativeRootBindingMode::LogicalRegisters);
    std::array<std::uint32_t, 128> ReadUserData(const Graphics::Commands::RootState& state) const;
    std::array<std::uint32_t, 128> Materialize(const Graphics::Commands::RootState& state, ShaderRecompiler::SrtRuntime runtime, ShaderRecompiler::ResourceSnapshot& snapshot) const;

private:
    const ShaderRecompiler::IrResourcePlan& plan;
    std::vector<std::uint32_t> registers;
    NativeRootArgumentWriter writer;
};
}
