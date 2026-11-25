#include "GraphicsWave.hpp"
#include "Validation.hpp"
#include <spirv/unified1/spirv.hpp>
#include <cstring>
#include <string_view>
#include <unordered_map>

namespace {
bool LocalCapability(std::uint32_t capability) {
    switch (capability) {
    case spv::CapabilityShader:
    case spv::CapabilityFloat16:
    case spv::CapabilityFloat64:
    case spv::CapabilityInt8:
    case spv::CapabilityInt16:
    case spv::CapabilityInt64:
    case spv::CapabilityClipDistance:
    case spv::CapabilityCullDistance:
    case spv::CapabilityPhysicalStorageBufferAddresses:
    case spv::CapabilityStorageBuffer8BitAccess:
    case spv::CapabilityStorageBuffer16BitAccess:
    case spv::CapabilityDenormPreserve:
    case spv::CapabilityDenormFlushToZero:
    case spv::CapabilitySignedZeroInfNanPreserve:
    case spv::CapabilityRoundingModeRTE:
    case spv::CapabilityRoundingModeRTZ:
        return true;
    default:
        return false;
    }
}

bool Independent(std::span<const std::uint32_t> code) {
    std::unordered_map<std::uint32_t, std::uint32_t> constants;
    for (std::size_t offset = 5; offset < code.size();) {
        auto instruction = code.subspan(offset, code[offset] >> 16);
        if ((instruction[0] & 65535) == spv::OpConstant && instruction.size() == 4) constants.emplace(instruction[2], instruction[3]);
        offset += instruction.size();
    }
    auto localScope = [&](std::uint32_t id) {
        auto found = constants.find(id);
        return found != constants.end() && (found->second == spv::ScopeDevice || found->second == spv::ScopeInvocation);
    };

    for (std::size_t offset = 5; offset < code.size();) {
        auto instruction = code.subspan(offset, code[offset] >> 16);
        auto opcode = instruction[0] & 65535;
        if (opcode == spv::OpCapability && !LocalCapability(instruction[1])) return false;
        if (opcode == spv::OpControlBarrier) return false;
        if (opcode == spv::OpMemoryBarrier && !localScope(instruction[1])) return false;
        if (opcode == spv::OpExtInstImport) {
            auto* name = reinterpret_cast<const char*>(instruction.data() + 2);
            auto length = (instruction.size() - 2) * 4;
            auto* end = static_cast<const char*>(std::memchr(name, 0, length));
            if (!end || std::string_view(name, static_cast<std::size_t>(end - name)) != "GLSL.std.450") return false;
        }
        if ((opcode >= spv::OpAtomicLoad && opcode <= spv::OpAtomicXor) || opcode == spv::OpAtomicFlagTestAndSet || opcode == spv::OpAtomicFlagClear) {
            auto index = opcode == spv::OpAtomicStore || opcode == spv::OpAtomicFlagClear ? 2u : 4u;
            if (!localScope(instruction[index])) return false;
        }
        offset += instruction.size();
    }
    return true;
}
}

void AnyBox::Shader::ValidateGraphicsWave(std::span<const std::uint32_t> code, std::uint32_t waveSize, const ShaderRecompiler::SpirvTargetOptions& target) {
    if ((waveSize != 32 && waveSize != 64) || (target.subgroupSize != 32 && target.subgroupSize != 64)) throw std::invalid_argument("Graphics shader wave or target subgroup size is invalid");
    ValidateSpirv(code, target.vulkanVersion);
    if (waveSize != target.subgroupSize && !Independent(code)) throw std::runtime_error("Graphics shader requires a matching subgroup width or supported cross-lane lowering");
}
