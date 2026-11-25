#include <anybox/shader/NativeVertex.hpp>
#include "Options.hpp"
#include "Optimize.hpp"
#include "Slices.hpp"
#include "Buffers.hpp"
#include <Optimization/SsaBuilder.hpp>

namespace {
void CancelDrawOffsets(ShaderRecompiler::IrProgram& program, AnyBox::Shader::NativeVertexSystemRegisters system) {
    using namespace ShaderRecompiler;
    auto offsetRegister = [&](const IrValue* value) {
        if (value->Opcode() != IrOpcode::GetUserData || value->ArgumentCount() != 1) return false;
        auto index = static_cast<std::int32_t>(value->Argument(0)->Register().index);
        return index == system.vertexOffset || index == system.instanceOffset;
    };
    for (const auto& block : program.Blocks()) {
        for (auto* value : block->Instructions()) {
            if (value->Opcode() != IrOpcode::IAdd32) continue;
            for (std::uint32_t index = 0; index < 2; ++index) {
                auto* subtract = value->Argument(index)->Resolve();
                if (subtract->Opcode() != IrOpcode::ISub32) continue;
                auto* offset = subtract->Argument(1)->Resolve();
                auto* other = value->Argument(1 - index)->Resolve();
                if (!offsetRegister(offset) || !offsetRegister(other) || offset->Argument(0)->Register() != other->Argument(0)->Register()) continue;
                value->ReplaceAllUsesWith(subtract->Argument(0)->Resolve());
                break;
            }
        }
    }
}
}

ShaderRecompiler::IrProgram AnyBox::Shader::LowerNativeVertex(const NativeFrontend& frontend, const Graphics::Container::NativeStageState& state, const ShaderRecompiler::ShaderVertexInputInfo& input, std::uint32_t userRegisters, const ShaderRecompiler::SpirvTargetOptions& target, NativeVertexSystemRegisters system) {
    using namespace ShaderRecompiler;
    if (state.stage != Graphics::Container::NativeHardwareStage::Vertex || input.logicalStage != IrShaderStage::Vertex) throw std::invalid_argument("Native vertex translation received a different hardware stage");
    if (target.subgroupSize != 32 && target.subgroupSize != 64) throw std::invalid_argument("Native vertex target subgroup size is invalid");
    if (input.hostSubgroupSize != target.subgroupSize) throw std::invalid_argument("Native vertex input and target subgroup sizes do not match");
    if (state.floatControl > 255) throw std::invalid_argument("Native vertex float mode is invalid");
    for (auto index : {system.vertexOffset, system.instanceOffset}) if (index < -1 || (index >= 0 && static_cast<std::uint32_t>(index) >= userRegisters)) throw std::invalid_argument("Native vertex draw offset register is outside its user data range");
    if (target.float32FlushToZero && (target.float32DenormPreserve || (state.floatControl & 0x30) != 0)) throw std::invalid_argument("Native vertex target float controls disagree with the initial mode");
    ShaderFloatMode mode{state.floatControl, state.dx10Clamp, state.ieeeMode, false};
    auto options = MakeNativeOptions(ShaderStageKind::Vertex, frontend.GetWaveSize(), userRegisters, mode, target);
    options.gcnLegacyVertexInputs = true;
    options.inputInfo.vertex = &input;
    EmbeddedFetchPlan offsets;
    offsets.vertexOffsetSgpr = system.vertexOffset;
    offsets.instanceOffsetSgpr = system.instanceOffset;
    if (system.vertexOffset >= 0 || system.instanceOffset >= 0) options.embeddedFetch = &offsets;
    auto result = TranslateNativeProgram(frontend.GetDecoded(), frontend.GetGraph(), options);
    SsaBuilder{}.Rewrite(result);
    OptimizeNativeProgram(result, frontend.GetWaveSize());
    if (system.vertexOffset >= 0 || system.instanceOffset >= 0) {
        CancelDrawOffsets(result, system);
        OptimizeNativeProgram(result, frontend.GetWaveSize());
    }
    MarkNativeBufferDescriptors(result);
    return result;
}

AnyBox::Shader::StageCompilation AnyBox::Shader::CompileNativeVertex(std::span<const std::byte> program, const ShaderRecompiler::ShaderVertexInputInfo& input, std::uint32_t userRegisters, const ShaderRecompiler::SpirvTargetOptions& target, const ShaderRecompiler::BindingLayout& layout) {
    auto state = Graphics::Container::ReadNativeStageState(program);
    NativeFrontend frontend(program);
    auto lowered = LowerNativeVertex(frontend, state, input, userRegisters, target);
    return CompileVertex(lowered, input, target, layout);
}
