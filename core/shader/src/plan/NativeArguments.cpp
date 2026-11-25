#include <anybox/shader/NativeShaderArguments.hpp>
#include <Optimization/ResourceMaterializer.hpp>
#include <algorithm>

namespace {
std::uint32_t Visibility(ShaderRecompiler::IrShaderStage stage) {
    using ShaderRecompiler::IrShaderStage;
    switch (stage) {
    case IrShaderStage::Compute: return 0;
    case IrShaderStage::Vertex: return 1;
    case IrShaderStage::Pixel: return 5;
    default: throw std::invalid_argument("Native resource arguments require a compute, vertex or pixel plan");
    }
}

bool SystemRegister(std::uint32_t index, ShaderRecompiler::IrShaderStage stage, std::span<const AnyBox::Graphics::Container::NativeRootBinding> roots) {
    if (stage == ShaderRecompiler::IrShaderStage::Compute || index >= 8) return false;
    return std::ranges::none_of(roots, [&](const auto& binding) { return index >= binding.firstUserRegister && index - binding.firstUserRegister < binding.registerCount; });
}

}

AnyBox::Shader::NativeShaderArguments::RegisterSelection AnyBox::Shader::NativeShaderArguments::selectRegisters(const ShaderRecompiler::IrResourcePlan& plan, std::span<const std::uint32_t> shaderRegisters, std::span<const Graphics::Container::NativeRootBinding> roots) {
    RegisterSelection result;
    for (auto index : ReadResourceRegisters(plan, shaderRegisters)) {
        if (SystemRegister(index, plan.stage, roots)) result.system.push_back(index);
        else result.root.push_back(index);
    }
    return result;
}

AnyBox::Shader::NativeShaderArguments::NativeShaderArguments(const ShaderRecompiler::IrResourcePlan& plan, const Graphics::Container::RootSignatureLayout& layout, std::span<const Graphics::Container::NativeRootBinding> roots, std::span<const std::uint32_t> shaderRegisters, NativeRootBindingMode mode) : plan(plan), selected(selectRegisters(plan, shaderRegisters, roots)), writer(layout, roots, selected.root, mode, Visibility(plan.stage)) {
}

std::array<std::uint32_t, 128> AnyBox::Shader::NativeShaderArguments::ReadUserData(const Graphics::Commands::RootState& state, std::span<const std::uint32_t> systemData) const {
    if (systemData.size() > 8 || (plan.stage == ShaderRecompiler::IrShaderStage::Compute && !systemData.empty())) throw std::invalid_argument("Native shader system register bank is invalid");
    for (auto index : selected.system) if (index >= systemData.size()) throw std::invalid_argument("Native graphics system register is missing");
    std::array<std::uint32_t, 128> compact;
    writer.Write(state, std::span(compact).first(selected.root.size()));
    std::array<std::uint32_t, 128> result{};
    for (std::size_t index = 0; index < selected.root.size(); ++index) result[selected.root[index]] = compact[index];
    for (auto index : selected.system) result[index] = systemData[index];
    return result;
}

std::array<std::uint32_t, 128> AnyBox::Shader::NativeShaderArguments::Materialize(const Graphics::Commands::RootState& state, ShaderRecompiler::SrtRuntime runtime, ShaderRecompiler::ResourceSnapshot& snapshot, std::span<const std::uint32_t> systemData) const {
    if (!runtime.userData.empty()) throw std::invalid_argument("Native shader resource runtime already supplies user registers");
    auto values = ReadUserData(state, systemData);
    runtime.userData = std::span(values).subspan(plan.userDataBase, plan.userDataCount);
    ShaderRecompiler::ResourceMaterializer{}.Materialize(plan, runtime, snapshot);
    if (!snapshot.srtPoison.empty() || snapshot.nullRootReads != 0) throw std::runtime_error("Native shader resource materialization encountered an unreadable or poisoned address");
    auto checkTables = [](const auto& tables) {
        for (const auto& table : tables) if (table.fault != 0 || table.outside) throw std::runtime_error("Native shader resource table materialization encountered an invalid descriptor");
    };
    checkTables(snapshot.tables.tables);
    checkTables(snapshot.tables.nested);
    return values;
}
