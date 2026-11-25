#include "Binary.hpp"
#include <algorithm>
#include <array>

void AnyBox::Shader::ValidateComputeArtifact(const ComputeArtifact& artifact) { ArtifactFormat::Validate(artifact); }

void AnyBox::Shader::ArtifactFormat::Validate(const ComputeArtifact& artifact) {
    if (artifact.profile != 1 || artifact.runtimeVersion != 14 || artifact.bdaVersion != 1) throw std::invalid_argument("Shader artifact target profile or runtime ABI is incompatible");
    if (artifact.waveSize != PreparedComputeWaveSize || (artifact.executionFlags & ~15u) != 0) throw std::invalid_argument("Shader artifact wave or execution flags are incompatible");
    std::uint64_t threads = 1;
    for (auto size : artifact.threads) {
        if (size == 0 || size > 1024) throw std::invalid_argument("Shader artifact workgroup dimension is invalid");
        threads *= size;
    }
    if (threads > 1024 || artifact.sharedMemoryBytes % 4 != 0) throw std::invalid_argument("Shader artifact workgroup size or shared memory alignment is invalid");
    if (artifact.pushConstantOffset > 128 || artifact.pushConstantSize > 128 - artifact.pushConstantOffset || ((artifact.pushConstantOffset | artifact.pushConstantSize) & 3) != 0) throw std::invalid_argument("Shader artifact push constant range is invalid");
    if (artifact.pushDataStart != UINT32_MAX && artifact.pushDataStart >= 32) throw std::invalid_argument("Shader artifact push data base is invalid");
    if (artifact.stencilReference != UINT32_MAX && artifact.stencilReference >= 32) throw std::invalid_argument("Shader artifact stencil data base is invalid");
    if (artifact.memoryOffsetCount > 128 || artifact.runtimeImageCount > 256 || artifact.runtimeStateDwords > 65536 || artifact.memoryOffset > 65536) throw std::invalid_argument("Shader artifact runtime data layout exceeds the ABI capacity");
    auto shaderDataWords = artifact.memoryOffset + (artifact.memoryOffsetCount + 3) / 4 + (artifact.dispatchThreadLimit ? 3 : 0) + artifact.runtimeImageCount * 12 + artifact.runtimeStateDwords;
    if (artifact.memoryOffset < artifact.userRegisters.size()) throw std::invalid_argument("Shader artifact runtime data overlaps its user registers");
    if (artifact.pushDataStart != UINT32_MAX && (artifact.pushDataStart < artifact.pushConstantOffset / 4 || artifact.pushDataStart + shaderDataWords > (artifact.pushConstantOffset + artifact.pushConstantSize) / 4)) throw std::invalid_argument("Shader artifact inline data exceeds its push constant range");
    std::array<bool, 128> selected{};
    for (auto index : artifact.userRegisters) {
        if (index >= selected.size() || selected[index]) throw std::invalid_argument("Shader artifact user register index is invalid or repeated");
        selected[index] = true;
    }
    if (artifact.buffers.size() > 128 || artifact.descriptors.size() > 258) throw std::length_error("Shader artifact resource count exceeds the ABI capacity");
    for (const auto& buffer : artifact.buffers) {
        if (static_cast<unsigned>(buffer.resourceClass) > 2 || static_cast<unsigned>(buffer.kind) > 2 || buffer.count == 0) throw std::invalid_argument("Shader artifact buffer declaration is invalid");
    }
    std::array<bool, 258> descriptors{};
    for (const auto& descriptor : artifact.descriptors) {
        if (descriptor.kind > 64 && descriptor.kind != 256 && descriptor.kind != 257) throw std::invalid_argument("Shader artifact descriptor binding kind is invalid");
        if (descriptors[descriptor.kind]) throw std::invalid_argument("Shader artifact descriptor binding kind is repeated");
        descriptors[descriptor.kind] = true;
        if (descriptor.resources.size() > UINT32_MAX) throw std::length_error("Shader artifact descriptor resource array is too large");
    }
    const auto& code = artifact.code;
    if (code.size() < 5 || code.size() > UINT32_MAX || code[0] != 0x07230203 || code[1] != 0x00010500 || code[3] == 0 || code[4] != 0) throw std::invalid_argument("Shader artifact SPIR-V header is invalid");
    for (std::size_t offset = 5; offset < code.size();) {
        auto count = code[offset] >> 16;
        if (count == 0 || count > code.size() - offset) throw std::invalid_argument("Shader artifact SPIR-V instruction is truncated");
        offset += count;
    }
}
