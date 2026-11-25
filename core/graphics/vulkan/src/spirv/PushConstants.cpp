#include "Layout.hpp"
#include <spirv/unified1/spirv.hpp>
#include <algorithm>
#include <cstring>

namespace {
VkShaderStageFlags Stage(std::uint32_t model) {
    switch (model) {
    case spv::ExecutionModelVertex: return VK_SHADER_STAGE_VERTEX_BIT;
    case spv::ExecutionModelTessellationControl: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    case spv::ExecutionModelTessellationEvaluation: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    case spv::ExecutionModelGeometry: return VK_SHADER_STAGE_GEOMETRY_BIT;
    case spv::ExecutionModelFragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
    case spv::ExecutionModelGLCompute: return VK_SHADER_STAGE_COMPUTE_BIT;
    default: throw std::runtime_error("SPIR-V push constant shader stage is not implemented");
    }
}
}

VkPushConstantRange AnyBox::Vulkan::Spirv::Layout::Read(std::string_view entryPoint) {
    if (entryPoint.empty()) throw std::invalid_argument("SPIR-V layout entry point is empty");
    std::unordered_set<std::uint32_t> interfaces;
    VkShaderStageFlags stage = 0;
    unsigned entries = 0;
    for (std::size_t offset = 5; offset < code.size();) {
        auto instruction = code.subspan(offset, code[offset] >> 16);
        if ((instruction[0] & 65535) == spv::OpEntryPoint) {
            ++entries;
            if (instruction.size() < 4) throw std::invalid_argument("SPIR-V entry point is truncated");
            auto* name = reinterpret_cast<const char*>(instruction.data() + 3);
            auto* end = static_cast<const char*>(std::memchr(name, 0, (instruction.size() - 3) * 4));
            if (!end) throw std::invalid_argument("SPIR-V entry point name is unterminated");
            auto length = static_cast<std::size_t>(end - name);
            if (std::string_view(name, length) == entryPoint) {
                if (stage) throw std::invalid_argument("SPIR-V entry point name is ambiguous");
                stage = Stage(instruction[1]);
                auto words = (length + 4) / 4;
                for (auto id : instruction.subspan(3 + words)) interfaces.insert(id);
            }
        }
        offset += instruction.size();
    }
    if (!stage) throw std::invalid_argument("SPIR-V layout entry point is missing");

    std::uint32_t block = 0;
    for (std::size_t offset = 5; offset < code.size();) {
        auto instruction = code.subspan(offset, code[offset] >> 16);
        if ((instruction[0] & 65535) == spv::OpVariable) {
            if (instruction.size() < 4) throw std::invalid_argument("SPIR-V variable is truncated");
            if (instruction[3] == spv::StorageClassPushConstant && (code[1] < 0x00010400 || interfaces.contains(instruction[2]))) {
                if (code[1] < 0x00010400 && entries != 1) throw std::runtime_error("SPIR-V push constant usage before version 1.4 with multiple entries is not implemented");
                if (block) throw std::invalid_argument("SPIR-V entry point declares multiple push constant blocks");
                auto pointer = type(instruction[1]);
                if (pointer.size() != 4 || (pointer[0] & 65535) != spv::OpTypePointer || pointer[2] != spv::StorageClassPushConstant) throw std::invalid_argument("SPIR-V push constant pointer type is invalid");
                block = pointer[3];
            }
        }
        offset += instruction.size();
    }
    if (!block) return {stage, 0, 0};
    auto structure = type(block);
    if ((structure[0] & 65535) != spv::OpTypeStruct || !blocks.contains(block)) throw std::invalid_argument("SPIR-V push constant variable has no block structure");
    auto end = size(block, {});
    std::uint32_t start = UINT32_MAX;
    for (std::size_t index = 2; index < structure.size(); ++index) start = std::min(start, *members.at((static_cast<std::uint64_t>(block) << 32) | (index - 2)).offset);
    auto alignedEnd = (end + 3) & ~std::uint64_t{3};
    start &= ~3u;
    if (alignedEnd > UINT32_MAX || alignedEnd <= start) throw std::length_error("SPIR-V push constant range is invalid or overflows");
    return {stage, start, static_cast<std::uint32_t>(alignedEnd - start)};
}

VkPushConstantRange AnyBox::Vulkan::ReadSpirvPushConstants(std::span<const std::uint32_t> code, std::string_view entryPoint) {
    return Spirv::Layout(code).Read(entryPoint);
}
