#include "../../src/native/PixelExports.hpp"
#include "NativeFixtures.hpp"
#include <IntermediateRepresentation/IrBuilder.hpp>

void CheckNativePixelExportSources() {
    using namespace ShaderRecompiler;
    using namespace Fixtures;
    for (auto compressed : {false, true}) {
        for (std::uint32_t mask = 0; mask < 16; ++mask) {
            IrProgram program;
            auto& block = program.CreateBlock();
            IrBuilder builder(program);
            builder.SetInsertionPoint(block);
            std::array<IrValue*, 4> components{};
            for (std::uint32_t index = 0; index < 4; ++index) components[index] = &builder.Constant(index + 1);
            auto& vector = builder.Emit(IrOpcode::CompositeConstructU32x4, IrType::U32x4, {components[0], components[1], components[2], components[3]});
            auto& active = builder.ConstantBool(true);
            program.Metadata().exportInfo = {{ExportTargetKind::Mrt, 0, 0, mask, true, compressed, true}, {ExportTargetKind::Mrt, 0, 0, 15, true, false, true}};
            auto& partial = builder.Emit(IrOpcode::SetAttribute, IrType::Void, {&vector, &active}, ExportFlags{0, 0});
            auto& full = builder.Emit(IrOpcode::SetAttribute, IrType::Void, {&vector, &active}, ExportFlags{1, 0});
            AnyBox::Shader::RemoveDisabledPixelExportSources(program);
            auto expected = compressed ? ((mask & 3) != 0 ? 1u : 0u) | ((mask & 12) != 0 ? 2u : 0u) : mask;
            auto* filtered = partial.Argument(0);
            Require(filtered->Opcode() == IrOpcode::CompositeConstructU32x4 && full.Argument(0) == &vector);
            for (std::uint32_t index = 0; index < 4; ++index) {
                Require(filtered->Argument(index)->ImmediateU32() == ((expected & (1u << index)) != 0 ? index + 1 : 0));
                Require(vector.Argument(index)->ImmediateU32() == index + 1);
            }
        }
    }
}
