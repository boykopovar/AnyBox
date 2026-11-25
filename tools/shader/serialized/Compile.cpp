#include "Prepare.hpp"
#include "../AuditTarget.hpp"
#include "../Compilation.hpp"
#include <anybox/graphics/container/Fspx.hpp>
#include <anybox/graphics/container/FspxDispatch.hpp>
#include <anybox/graphics/container/FspxMetadata.hpp>
#include <anybox/shader/NativeCompute.hpp>
#include <anybox/shader/NativeRootArgumentWriter.hpp>
#include <anybox/shader/SerializedComputeSource.hpp>
#include <anybox/shader/SerializedRootArguments.hpp>
#include <Optimization/ResourceMaterializer.hpp>
#include <algorithm>

AnyBox::Shader::Tools::PreparedSerializedCompute AnyBox::Shader::Tools::CompileSerializedCompute(std::span<const std::byte> data, std::span<const std::byte> root, std::uint32_t subgroupSize) {
    using namespace Graphics::Container;
    FspxView view(data);
    auto state = ReadSerializedComputeState(view.GetDispatchState());
    auto metadata = ReadSerializedComputeMetadata(view);
    auto layout = ReadRootSignatureLayout(root);
    auto roots = BuildSerializedComputeRoots(metadata.resources, layout);
    auto identity = HashSerializedComputeSource({view.GetDispatchState(), view.GetAuxiliaryState(), view.GetCode(), view.GetRecords()}, HashSerializedRootSignature(root), subgroupSize);
    auto target = AuditTarget();
    target.subgroupSize = subgroupSize;
    NativeFrontend frontend(view.GetCode(), state.waveSize);
    auto translated = LowerNativeCompute(frontend, state.shader, ShaderRecompiler::ShaderFloatMode{state.shader.floatControl, state.shader.dx10Clamp, state.shader.ieeeMode, false}, target);
    auto compiled = CompileCompute(translated.program, translated.input, target, {0, 0, 0, 128});
    auto plan = ShaderRecompiler::ResourceMaterializer{}.ExtractPlan(translated.program);

    auto registers = compiled.bindings.layout.userDataRegisters;
    for (std::size_t index = 0; index < plan.valueStorage.Size(); ++index) {
        const auto& value = plan.valueStorage[index];
        if (value.Opcode() != ShaderRecompiler::IrOpcode::GetUserData) continue;
        if (value.ArgumentCount() != 1) throw std::invalid_argument("Prepared serialized compute user register expression is invalid");
        auto reg = static_cast<std::uint32_t>(value.Argument(0)->Register().index);
        if (std::ranges::find(registers, reg) == registers.end()) registers.push_back(reg);
    }
    (void)NativeRootArgumentWriter(layout, roots, registers);
    auto artifact = MakeArtifact(std::move(compiled), {}, translated.input, identity);
    ValidateComputeArtifact(artifact);
    return {std::move(artifact), std::move(plan)};
}
