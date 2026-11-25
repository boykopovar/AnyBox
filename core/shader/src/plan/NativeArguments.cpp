#include <anybox/shader/NativeComputeArguments.hpp>
#include <Optimization/ResourceMaterializer.hpp>

AnyBox::Shader::NativeComputeArguments::NativeComputeArguments(const ShaderRecompiler::IrResourcePlan& plan, const Graphics::Container::RootSignatureLayout& layout, std::span<const Graphics::Container::NativeRootBinding> roots, std::span<const std::uint32_t> shaderRegisters, NativeRootBindingMode mode) : plan(plan), registers(ReadResourceRegisters(plan, shaderRegisters)), writer(layout, roots, registers, mode) {
    if (plan.stage != ShaderRecompiler::IrShaderStage::Compute) throw std::invalid_argument("Native compute arguments require a compute resource plan");
}

std::array<std::uint32_t, 128> AnyBox::Shader::NativeComputeArguments::ReadUserData(const Graphics::Commands::RootState& state) const {
    std::array<std::uint32_t, 128> compact{};
    writer.Write(state, std::span(compact).first(registers.size()));
    std::array<std::uint32_t, 128> result{};
    for (std::size_t index = 0; index < registers.size(); ++index) result[registers[index]] = compact[index];
    return result;
}

std::array<std::uint32_t, 128> AnyBox::Shader::NativeComputeArguments::Materialize(const Graphics::Commands::RootState& state, ShaderRecompiler::SrtRuntime runtime, ShaderRecompiler::ResourceSnapshot& snapshot) const {
    if (!runtime.userData.empty()) throw std::invalid_argument("Native compute resource runtime already supplies user registers");
    auto values = ReadUserData(state);
    runtime.userData = std::span(values).subspan(plan.userDataBase, plan.userDataCount);
    ShaderRecompiler::ResourceMaterializer{}.Materialize(plan, runtime, snapshot);
    if (!snapshot.srtPoison.empty() || snapshot.nullRootReads != 0) throw std::runtime_error("Native compute resource materialization encountered an unreadable or poisoned address");
    auto checkTables = [](const auto& tables) {
        for (const auto& table : tables) if (table.fault != 0 || table.outside) throw std::runtime_error("Native compute resource table materialization encountered an invalid descriptor");
    };
    checkTables(snapshot.tables.tables);
    checkTables(snapshot.tables.nested);
    return values;
}
