#include <anybox/shader/NativeVertex.hpp>
#include "../buffer/Target.hpp"
#include "NativeFixtures.hpp"
#include <array>

void CheckSpirvValidation(std::span<const std::uint32_t> code);
void CheckGraphicsWaveProof(std::span<const std::uint32_t> code, std::uint32_t waveSize, const ShaderRecompiler::SpirvTargetOptions& target);

namespace {
std::vector<std::byte> VertexProgram(bool wide) {
    auto data = Fixtures::MakeComputeProgram();
    data.resize(0x1C0);
    Fixtures::Set(data, 0x100, 4);
    Fixtures::Set(data, 0xC8, 0x80);
    Fixtures::Set(data, 0xD4, wide ? 0 : 0x20);
    for (std::size_t offset = 0x168; offset < data.size(); offset += 4) Fixtures::Set(data, offset, 0xBF9F0000);
    constexpr std::array<std::uint32_t, 8> instructions{0x7E020280, 0x7E0402FF, 0x3F800000, 0xF80008CF, 0x02010100, 0xF800020F, 0x02010100, 0xBF810000};
    for (std::size_t index = 0; index < instructions.size(); ++index) Fixtures::Set(data, 0x168 + index * 4, instructions[index]);
    return data;
}
}

void CheckNativeVertex() {
    using namespace AnyBox::Shader;
    using namespace ShaderRecompiler;
    using namespace Fixtures;
    for (bool wide : {false, true}) {
        auto data = VertexProgram(wide);
        auto target = BufferTests::Target();
        std::vector<std::uint32_t> capabilities(target.supportedCapabilities.begin(), target.supportedCapabilities.end());
        capabilities.push_back(spv::CapabilityClipDistance);
        target.supportedCapabilities = capabilities;
        target.subgroupSize = wide ? 64 : 32;
        ShaderVertexInputInfo input;
        input.hostSubgroupSize = target.subgroupSize;
        auto compiled = CompileNativeVertex(data, input, 0, target, {0, 0, 0, 128});
        CheckSpirvValidation(compiled.code);
        bool vertex = false;
        bool vertexIndex = false;
        bool parameter = false;
        for (std::size_t offset = 5; offset < compiled.code.size();) {
            auto count = compiled.code[offset] >> 16;
            auto opcode = compiled.code[offset] & 65535;
            Require(count != 0 && count <= compiled.code.size() - offset);
            if (opcode == 15) vertex = compiled.code[offset + 1] == 0;
            if (opcode == 71 && count >= 4 && compiled.code[offset + 2] == 11 && compiled.code[offset + 3] == 42) vertexIndex = true;
            if (opcode == 71 && count >= 4 && compiled.code[offset + 2] == 30 && compiled.code[offset + 3] == 0) parameter = true;
            offset += count;
        }
        Require(vertex && vertexIndex && parameter);
        auto state = AnyBox::Graphics::Container::ReadNativeStageState(data);
        NativeFrontend frontend(data);
        auto lowered = LowerNativeVertex(frontend, state, input, 0, target);
        Require(lowered.Resources().stage == IrShaderStage::Vertex && lowered.WaveSize() == target.subgroupSize);
        auto bufferData = data;
        constexpr std::array<std::uint32_t, 11> bufferInstructions{0x7E020280, 0x7E0402FF, 0x3F800000, 0xF4200100, 0xFA000000, 0x7E000204, 0xF80008CF, 0x02010100, 0xF800020F, 0x02010100, 0xBF810000};
        for (std::size_t index = 0; index < bufferInstructions.size(); ++index) Set(bufferData, 0x168 + index * 4, bufferInstructions[index]);
        NativeFrontend bufferFrontend(bufferData);
        auto bufferProgram = LowerNativeVertex(bufferFrontend, state, input, 4, target);
        bool scalarBuffer = false;
        for (const auto& memory : bufferProgram.Resources().memoryInfo) {
            if (memory.kind == ResourceKind::ScalarBuffer) {
                Require(memory.gpuDescriptor);
                scalarBuffer = true;
            }
        }
        Require(scalarBuffer);
        auto bufferCompiled = CompileVertex(bufferProgram, input, target, {0, 0, 0, 128});
        CheckSpirvValidation(bufferCompiled.code);
        CheckGraphicsWaveProof(bufferCompiled.code, wide ? 64u : 32u, target);
        auto incompatible = target;
        incompatible.subgroupSize = wide ? 32 : 64;
        auto adjustedInput = input;
        adjustedInput.hostSubgroupSize = incompatible.subgroupSize;
        auto adjusted = CompileVertex(lowered, adjustedInput, incompatible, {0, 0, 0, 128});
        CheckSpirvValidation(adjusted.code);
        input.hostSubgroupSize = incompatible.subgroupSize;
        Reject<std::invalid_argument>([&] { LowerNativeVertex(frontend, state, input, 0, target); });
        input.hostSubgroupSize = target.subgroupSize;
        state.stage = AnyBox::Graphics::Container::NativeHardwareStage::Compute;
        Reject<std::invalid_argument>([&] { LowerNativeVertex(frontend, state, input, 0, target); });
        state = AnyBox::Graphics::Container::ReadNativeStageState(data);
        state.floatControl = 256;
        Reject<std::invalid_argument>([&] { LowerNativeVertex(frontend, state, input, 0, target); });
        state.floatControl = 0xC0;
        input.logicalStage = IrShaderStage::Mesh;
        Reject<std::invalid_argument>([&] { LowerNativeVertex(frontend, state, input, 0, target); });
    }
}
