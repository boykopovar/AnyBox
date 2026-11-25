#include "Layout.hpp"
#include <spirv/unified1/spirv.hpp>

AnyBox::Vulkan::Spirv::Layout::Layout(Words code) : code(code) {
    if (code.size() < 5 || code[0] != spv::MagicNumber || code[3] == 0 || code[4] != 0) throw std::invalid_argument("SPIR-V layout header is invalid");
    for (std::size_t offset = 5; offset < code.size();) {
        auto count = code[offset] >> 16;
        if (count == 0 || count > code.size() - offset) throw std::invalid_argument("SPIR-V layout instruction is truncated");
        auto instruction = code.subspan(offset, count);
        auto opcode = instruction[0] & 65535;
        if (opcode >= spv::OpTypeVoid && opcode <= spv::OpTypeForwardPointer) {
            if (count < 2 || instruction[1] == 0 || instruction[1] >= code[3]) throw std::invalid_argument("SPIR-V layout type identifier is invalid");
            if (opcode != spv::OpTypeForwardPointer && !types.emplace(instruction[1], instruction).second) throw std::invalid_argument("SPIR-V layout type is duplicated");
        } else if (opcode == spv::OpConstant) {
            if (count < 4 || !constants.emplace(instruction[2], instruction).second) throw std::invalid_argument("SPIR-V layout constant is invalid");
        } else if (opcode == spv::OpDecorate) {
            if (count < 3) throw std::invalid_argument("SPIR-V layout decoration is truncated");
            if (instruction[2] == spv::DecorationBlock) blocks.insert(instruction[1]);
            if (instruction[2] == spv::DecorationArrayStride) {
                if (count != 4 || instruction[3] == 0 || !strides.emplace(instruction[1], instruction[3]).second) throw std::invalid_argument("SPIR-V array stride is invalid");
            }
        } else if (opcode == spv::OpMemberDecorate) {
            if (count < 4) throw std::invalid_argument("SPIR-V member decoration is truncated");
            auto& member = members[(static_cast<std::uint64_t>(instruction[1]) << 32) | instruction[2]];
            if (instruction[3] == spv::DecorationOffset || instruction[3] == spv::DecorationMatrixStride) {
                auto& field = instruction[3] == spv::DecorationOffset ? member.offset : member.matrixStride;
                if (count != 5 || field.has_value()) throw std::invalid_argument("SPIR-V member layout is invalid or duplicated");
                field = instruction[4];
            }
            if (instruction[3] == spv::DecorationRowMajor) member.rowMajor = true;
        }
        offset += count;
    }
}

AnyBox::Vulkan::Spirv::Words AnyBox::Vulkan::Spirv::Layout::type(std::uint32_t id) const {
    auto found = types.find(id);
    if (found == types.end()) throw std::invalid_argument("SPIR-V layout references an undefined type");
    return found->second;
}

std::uint64_t AnyBox::Vulkan::Spirv::Layout::constant(std::uint32_t id) const {
    auto found = constants.find(id);
    if (found == constants.end()) throw std::runtime_error("SPIR-V specialized array length is not implemented");
    auto value = found->second;
    auto declaration = type(value[1]);
    if ((declaration[0] & 65535) != spv::OpTypeInt || declaration.size() != 4 || (declaration[2] != 32 && declaration[2] != 64)) throw std::invalid_argument("SPIR-V array length is not a supported integer");
    if (value.size() != 3 + declaration[2] / 32) throw std::invalid_argument("SPIR-V array length constant is truncated");
    auto result = static_cast<std::uint64_t>(value[3]);
    if (declaration[2] == 64) result |= static_cast<std::uint64_t>(value[4]) << 32;
    if (result == 0 || result > UINT32_MAX) throw std::invalid_argument("SPIR-V array length is invalid");
    return result;
}
