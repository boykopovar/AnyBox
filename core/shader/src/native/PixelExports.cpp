#include "PixelExports.hpp"
#include <IntermediateRepresentation/IrBuilder.hpp>

void AnyBox::Shader::RemoveDisabledPixelExportSources(ShaderRecompiler::IrProgram& program) {
    using namespace ShaderRecompiler;
    IrBuilder builder(program);
    auto& zero = builder.Constant(0u);
    for (const auto& block : program.Blocks()) {
        for (auto* instruction : block->Instructions()) {
            if (instruction->Opcode() != IrOpcode::SetAttribute) continue;
            const auto& info = program.Metadata().exportInfo.at(instruction->Flags<ExportFlags>().index);
            if (info.kind != ExportTargetKind::Mrt) continue;
            auto mask = info.compr ? ((info.en & 3u) != 0 ? 1u : 0u) | ((info.en & 12u) != 0 ? 2u : 0u) : info.en;
            if (mask == 15) continue;
            auto* vector = instruction->Argument(0)->Resolve();
            if (vector->Opcode() != IrOpcode::CompositeConstructU32x4 || vector->ArgumentCount() != 4) continue;
            auto& filtered = program.CreateValue(IrOpcode::CompositeConstructU32x4, IrType::U32x4);
            for (std::uint32_t component = 0; component < 4; ++component) filtered.AddArgument((mask & (1u << component)) != 0 ? vector->Argument(component) : &zero);
            block->InsertInstructionBefore(instruction, &filtered);
            instruction->ReplaceArgument(0, &filtered);
        }
    }
}
