#include "../../src/backend/GraphicsWave.hpp"
#include "NativeFixtures.hpp"
#include <spirv/unified1/spirv.hpp>
#include <array>

void CheckGraphicsWaveProof(std::span<const std::uint32_t> code, std::uint32_t waveSize, const ShaderRecompiler::SpirvTargetOptions& target) {
    using namespace AnyBox::Shader;
    using namespace Fixtures;
    auto other = target;
    other.subgroupSize = waveSize == 64 ? 32 : 64;
    ValidateGraphicsWave(code, waveSize, other);
    std::vector<std::uint32_t> subgroup(code.begin(), code.end());
    std::array<std::uint32_t, 2> capability{(2u << 16) | spv::OpCapability, spv::CapabilityGroupNonUniform};
    subgroup.insert(subgroup.begin() + 5, capability.begin(), capability.end());
    Reject<std::runtime_error>([&] { ValidateGraphicsWave(subgroup, waveSize, other); });
    ValidateGraphicsWave(subgroup, waveSize, target);

    std::uint32_t scope = 0;
    std::uint32_t scopeType = 0;
    for (std::size_t offset = 5; offset < code.size();) {
        if ((code[offset] & 65535) == spv::OpAtomicOr) scope = code[offset + 4];
        offset += code[offset] >> 16;
    }
    Require(scope != 0);
    for (std::size_t offset = 5; offset < code.size();) {
        if ((code[offset] & 65535) == spv::OpConstant && code[offset + 2] == scope) scopeType = code[offset + 1];
        offset += code[offset] >> 16;
    }
    Require(scopeType != 0);
    std::vector<std::uint32_t> atomic(code.begin(), code.end());
    auto newScope = atomic[3]++;
    for (std::size_t offset = 5; offset < atomic.size();) {
        auto opcode = atomic[offset] & 65535;
        if (opcode == spv::OpAtomicOr) atomic[offset + 4] = newScope;
        offset += atomic[offset] >> 16;
    }
    std::array<std::uint32_t, 4> constant{(4u << 16) | spv::OpConstant, scopeType, newScope, spv::ScopeSubgroup};
    for (std::size_t offset = 5; offset < atomic.size();) {
        if ((atomic[offset] & 65535) == spv::OpFunction) {
            atomic.insert(atomic.begin() + static_cast<std::ptrdiff_t>(offset), constant.begin(), constant.end());
            break;
        }
        offset += atomic[offset] >> 16;
    }
    ValidateGraphicsWave(atomic, waveSize, target);
    Reject<std::runtime_error>([&] { ValidateGraphicsWave(atomic, waveSize, other); });
    other.subgroupSize = 16;
    Reject<std::invalid_argument>([&] { ValidateGraphicsWave(code, waveSize, other); });
}
