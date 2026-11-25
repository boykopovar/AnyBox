#include "Buffers.hpp"
#include <Recompiler.hpp>
#include <stdexcept>

void AnyBox::Shader::PrepareNativeAddressLoads(ShaderRecompiler::IrProgram& program) {
    using namespace ShaderRecompiler;
    for (const auto& block : program.Blocks()) {
        for (const auto* value : block->Instructions()) {
            if (value->Opcode() != IrOpcode::LoadAddressU32) continue;
            auto& memory = program.Resources().memoryInfo.at(value->Flags<MemoryFlags>().index);
            if (memory.kind != ResourceKind::ScalarAddress) continue;
            if (memory.dataBits != 32 || memory.dataDwords != 1 || memory.componentCount == 0 || memory.componentCount > 16 || (memory.componentCount & (memory.componentCount - 1)) != 0 || memory.componentIndex >= memory.componentCount) throw std::invalid_argument("Native scalar address load grouping is invalid");
            memory.kind = ResourceKind::Global;
            memory.componentCount = 1;
            memory.componentIndex = 0;
        }
    }
}

void AnyBox::Shader::MarkNativeBufferDescriptors(ShaderRecompiler::IrProgram& program) {
    using namespace ShaderRecompiler;
    for (const auto& block : program.Blocks()) {
        for (auto* value : block->Instructions()) {
            if (BufferAccessOf(value->Opcode()) == BufferAccess::None) continue;
            auto& memory = program.Resources().memoryInfo.at(value->Flags<MemoryFlags>().index);
            if (memory.kind == ResourceKind::Buffer || memory.kind == ResourceKind::ScalarBuffer) memory.gpuDescriptor = true;
        }
    }
}
