#include "Prologue.hpp"
#include <RdnaDecoder/RdnaInstructionDecoder.hpp>
#include <algorithm>
#include <array>
#include <stdexcept>

bool AnyBox::Shader::NormalizeNativeHashPrologue(std::span<const std::uint32_t> code, std::vector<std::uint32_t>& normalized) {
    if (code.size() < 9) return false;
    for (std::size_t index = 0; index < 6; index += 3) {
        if ((code[index] & 0xFFFF0000u) != 0xD5800000u || (code[index + 1] & 0xFFFF0000u) != 0x03FE0000u) return false;
    }
    if (code[6] != 0xD5800000u || code[7] != 0x03FE0100u) return false;
    normalized.assign(code.begin(), code.end());
    std::fill_n(normalized.begin(), 9, 0xBF800000u);
    return true;
}

void AnyBox::Shader::RestoreNativeHashPrologue(ShaderRecompiler::RdnaProgram& program, std::span<const std::uint32_t> code) {
    using namespace ShaderRecompiler;
    if (program.instructions.size() < 9) throw std::logic_error("Normalized native hash prologue is incomplete");
    std::array<RdnaInstruction, 3> prologue{};
    for (std::size_t index = 0; index < prologue.size(); ++index) {
        auto& instruction = prologue[index];
        instruction.programCounter = static_cast<std::uint32_t>(index * 12);
        instruction.family = RdnaInstructionFamily::VOP3;
        instruction.opcodeId = 0x180;
        instruction.op = RdnaOpcode::VNop;
        instruction.destination.kind = RdnaOperandKind::Null;
        SetRdnaRawWords(instruction, code, static_cast<std::uint32_t>(index * 3), 3);
    }
    program.instructions.erase(program.instructions.begin(), program.instructions.begin() + 9);
    program.instructions.insert(program.instructions.begin(), prologue.begin(), prologue.end());
}
