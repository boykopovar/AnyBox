#include "Decode.hpp"
#include "Prologue.hpp"
#include <RdnaDecoder/RdnaInstructionDecoder.hpp>
#include <stdexcept>
#include <vector>

namespace {
bool XboxPermlane(std::span<const std::uint32_t> code, std::size_t index) {
    auto first = code[index];
    auto opcode = first & 0xFFFF0000;
    return (opcode == 0xD7770000 || opcode == 0xD7780000) && (first & 0x4000) != 0 && (first & 0xA700) == 0 && (code[index + 1] & 0xF8000000) == 0;
}
}

ShaderRecompiler::RdnaProgram AnyBox::Shader::DecodeNativeProgram(std::span<const std::uint32_t> code) {
    using namespace ShaderRecompiler;
    RdnaProgram program;
    std::vector<std::uint32_t> normalized;
    bool hashPrologue = NormalizeNativeHashPrologue(code, normalized);
    for (;;) {
        auto input = normalized.empty() ? code : std::span<const std::uint32_t>(normalized);
        try {
            DecodeRdnaProgram(input, program);
            break;
        } catch (const std::invalid_argument&) {
            std::size_t next = 0;
            if (!program.instructions.empty()) {
                const auto& last = program.instructions.back();
                if (last.op == RdnaOpcode::SBranch || last.op == RdnaOpcode::SEndpgm || last.op == RdnaOpcode::SCodeEnd) throw;
                next = static_cast<std::size_t>(last.programCounter / 4) + last.wordCount;
            }
            if (next >= input.size() || input.size() - next < 2) throw;
            bool legacyFma = (input[next] & 0xFFFF0000) == 0xD5400000;
            bool permlane = XboxPermlane(input, next);
            if (!legacyFma && !permlane) throw;
            if (normalized.empty()) normalized.assign(code.begin(), code.end());
            if (legacyFma) normalized[next] = (normalized[next] & 0x0000FFFF) | 0xD54B0000;
            else normalized[next] &= ~0x4000u;
        }
    }
    if (hashPrologue) RestoreNativeHashPrologue(program, code);
    program.code = code;
    for (auto& instruction : program.instructions) {
        auto index = instruction.programCounter / 4;
        if (instruction.family != RdnaInstructionFamily::VOP3) continue;
        if ((code[index] & 0xFFFF0000) == 0xD5400000) {
            instruction.op = RdnaOpcode::VMadLegacyF32;
            instruction.opcodeId = 0x140;
            SetRdnaRawWords(instruction, code, index, instruction.wordCount);
        } else if (instruction.op == RdnaOpcode::VPermlane16B32 || instruction.op == RdnaOpcode::VPermlanex16B32) {
            SetRdnaRawWords(instruction, code, index, instruction.wordCount);
        }
    }
    return program;
}
