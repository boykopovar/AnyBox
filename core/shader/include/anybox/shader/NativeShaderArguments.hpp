#pragma once

#include "NativeRootArgumentWriter.hpp"
#include "ResourcePlan.hpp"
#include <Optimization/SrtWalker.hpp>

namespace AnyBox::Shader {
class NativeShaderArguments {
public:
    NativeShaderArguments(const ShaderRecompiler::IrResourcePlan& plan, const Graphics::Container::RootSignatureLayout& layout, std::span<const Graphics::Container::NativeRootBinding> roots, std::span<const std::uint32_t> shaderRegisters, NativeRootBindingMode mode = NativeRootBindingMode::LogicalRegisters);
    std::array<std::uint32_t, 128> ReadUserData(const Graphics::Commands::RootState& state, std::span<const std::uint32_t> systemData = {}) const;
    std::array<std::uint32_t, 128> Materialize(const Graphics::Commands::RootState& state, ShaderRecompiler::SrtRuntime runtime, ShaderRecompiler::ResourceSnapshot& snapshot, std::span<const std::uint32_t> systemData = {}) const;
    std::span<const std::uint32_t> GetSystemRegisters() const noexcept { return selected.system; }

private:
    struct RegisterSelection {
        std::vector<std::uint32_t> root;
        std::vector<std::uint32_t> system;
    };
    static RegisterSelection selectRegisters(const ShaderRecompiler::IrResourcePlan& plan, std::span<const std::uint32_t> shaderRegisters, std::span<const Graphics::Container::NativeRootBinding> roots);
    const ShaderRecompiler::IrResourcePlan& plan;
    RegisterSelection selected;
    NativeRootArgumentWriter writer;
};
}
