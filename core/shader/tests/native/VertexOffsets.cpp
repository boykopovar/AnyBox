#include <anybox/shader/NativeVertex.hpp>
#include "../buffer/Target.hpp"
#include "Fixtures.hpp"

void CheckSpirvValidation(std::span<const std::uint32_t> code);

namespace {
struct Inputs {
    std::uint32_t vertex;
    std::uint32_t instance;
    std::uint32_t vertexOffset;
    std::uint32_t instanceOffset;
};

std::uint32_t Evaluate(const ShaderRecompiler::IrValue* value, const Inputs& inputs) {
    using namespace ShaderRecompiler;
    value = value->Resolve();
    if (value->HasImmediate()) return value->ImmediateU32();
    auto read = [&](std::uint32_t index) { return Evaluate(value->Argument(index), inputs); };
    switch (value->Opcode()) {
    case IrOpcode::IAdd32: return read(0) + read(1);
    case IrOpcode::ISub32: return read(0) - read(1);
    case IrOpcode::GetUserData:
        if (value->Argument(0)->Register().index == 2) return inputs.vertexOffset;
        if (value->Argument(0)->Register().index == 3) return inputs.instanceOffset;
        break;
    case IrOpcode::GetBuiltin:
        if (read(0) == static_cast<std::uint32_t>(StageInputKind::VertexIndex)) return inputs.vertex;
        if (read(0) == static_cast<std::uint32_t>(StageInputKind::InstanceIndex)) return inputs.instance;
        break;
    default: break;
    }
    throw std::runtime_error("Native vertex offset test encountered an unexpected expression");
}

void CheckOutputs(const ShaderRecompiler::IrProgram& program, bool vertexAdjusted, bool instanceAdjusted) {
    using namespace ShaderRecompiler;
    using namespace Fixtures;
    std::uint32_t count = 0;
    for (const auto& block : program.Blocks()) {
        for (auto* instruction : block->Instructions()) {
            if (instruction->Opcode() != IrOpcode::SetAttribute) continue;
            auto* vector = instruction->Argument(0)->Resolve();
            Require(vector->Opcode() == IrOpcode::CompositeConstructU32x4);
            for (const auto& values : std::array<Inputs, 4>{{{23, 17, 0, 0}, {23, 17, 7, 11}, {23, 17, UINT32_MAX - 6, 3}, {0, 0, UINT32_MAX, UINT32_MAX}}}) {
                Require(Evaluate(vector->Argument(0), values) == values.vertex + (vertexAdjusted ? 0u : values.vertexOffset));
                Require(Evaluate(vector->Argument(1), values) == values.instance + (instanceAdjusted ? 0u : values.instanceOffset));
            }
            ++count;
        }
    }
    Require(count == 1);
}
}

void CheckNativeVertexOffsets() {
    using namespace AnyBox::Shader;
    using namespace ShaderRecompiler;
    using namespace Fixtures;
    const std::array<std::uint32_t, 5> words{0x4A000002, 0x4A020203, 0xF80008CF, 0x01010100, 0xBF810000};
    NativeFrontend frontend(std::as_bytes(std::span(words)), 32);
    AnyBox::Graphics::Container::NativeStageState state{AnyBox::Graphics::Container::NativeHardwareStage::Vertex, 0xC0, true, false};
    auto target = BufferTests::Target();
    std::vector<std::uint32_t> capabilities(target.supportedCapabilities.begin(), target.supportedCapabilities.end());
    capabilities.push_back(spv::CapabilityClipDistance);
    target.supportedCapabilities = capabilities;
    ShaderVertexInputInfo input;
    input.hostSubgroupSize = target.subgroupSize;
    auto original = LowerNativeVertex(frontend, state, input, 4, target);
    CheckOutputs(original, false, false);
    auto adjusted = LowerNativeVertex(frontend, state, input, 4, target, {2, 3});
    CheckOutputs(adjusted, true, true);
    auto vertexOnly = LowerNativeVertex(frontend, state, input, 4, target, {2, -1});
    CheckOutputs(vertexOnly, true, false);
    auto instanceOnly = LowerNativeVertex(frontend, state, input, 4, target, {-1, 3});
    CheckOutputs(instanceOnly, false, true);
    auto compiled = CompileVertex(adjusted, input, target, {0, 0, 0, 128});
    CheckSpirvValidation(compiled.code);
    Require(compiled.bindings.layout.userDataRegisters.empty());
    for (const auto& registers : std::array<NativeVertexSystemRegisters, 4>{{{-2, 3}, {2, -2}, {4, 3}, {2, 4}}}) Reject<std::invalid_argument>([&] { (void)LowerNativeVertex(frontend, state, input, 4, target, registers); });
}
