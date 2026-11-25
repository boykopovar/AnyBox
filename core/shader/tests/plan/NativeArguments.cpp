#include <anybox/shader/NativeComputeArguments.hpp>
#include "../native/RootFixture.hpp"
#include <Optimization/ResourceMaterializer.hpp>
#include <algorithm>

namespace {
class Memory {
public:
    static bool Read(void* context, std::uint64_t address, std::uint32_t* output) {
        auto& memory = *static_cast<Memory*>(context);
        if (!memory.readable || address != 0x10000 || !output) return false;
        ++memory.reads;
        *output = memory.value;
        return true;
    }
    std::uint32_t value = 0x12345678;
    std::uint32_t reads = 0;
    bool readable = true;
};
}

void CheckNativeComputeArguments() {
    using namespace AnyBox::Shader;
    using namespace AnyBox::Graphics::Container;
    using namespace ShaderRecompiler;
    using namespace Fixtures;
    NativeTests::RootFixture fixture(32, 16);
    auto translation = fixture.Translate();
    (void)CompileCompute(translation.program, translation.input, BufferTests::Target(), {0, 0, 0, 128});
    auto plan = ResourceMaterializer{}.ExtractPlan(translation.program);
    auto bytes = EncodeResourcePlan(plan, {});
    plan = DecodeResourcePlan(bytes, {}, IrShaderStage::Compute);
    RootSignatureLayout layout{2, 0, 0, {{RootParameterKind::ConstantBuffer, 0, 0, 0, 0, 0, {}}, {RootParameterKind::Constants, 0, 1, 0, 10, 0, {}}}};
    std::array<NativeRootBinding, 2> roots{{{NativeRootKind::Descriptor, 0, 2, 0, 0, 0}, {NativeRootKind::Constants, 0, 10, 1, 7, 1}}};
    std::array<std::uint32_t, 2> shaderRegisters{16, 7};
    Require(ReadResourceRegisters(plan, shaderRegisters) == std::vector<std::uint32_t>{0, 1, 7, 16});
    NativeComputeArguments arguments(plan, layout, roots, shaderRegisters);
    AnyBox::Graphics::Commands::RootState state(layout);
    Reject<std::logic_error>([&] { (void)arguments.ReadUserData(state); });
    state.Write(0, 0, std::array{0x10000u, 0u});
    state.Write(1, 0, std::array<std::uint32_t, 10>{11, 0, 0, 0, 0, 0, 0, 0, 0, 99});
    auto values = arguments.ReadUserData(state);
    Require(values[0] == 0x10000 && values[1] == 0 && values[7] == 11 && values[16] == 99);
    Require(std::ranges::count(values, 0u) == 125);
    Memory memory;
    SrtRuntime runtime;
    runtime.userContext = &memory;
    runtime.readMemory = &Memory::Read;
    ResourceSnapshot snapshot;
    Require(arguments.Materialize(state, runtime, snapshot) == values);
    Require(memory.reads != 0 && std::ranges::find(snapshot.flattenedSrt, memory.value) != snapshot.flattenedSrt.end());
    memory.value = 0xABCDEF12;
    (void)arguments.Materialize(state, runtime, snapshot);
    Require(std::ranges::find(snapshot.flattenedSrt, memory.value) != snapshot.flattenedSrt.end());
    runtime.userData = values;
    Reject<std::invalid_argument>([&] { (void)arguments.Materialize(state, runtime, snapshot); });
    runtime.userData = {};
    memory.readable = false;
    Reject<std::runtime_error>([&] { (void)arguments.Materialize(state, runtime, snapshot); });
    memory.readable = true;
    state.Write(0, 0, std::array{0u, 0u});
    Reject<std::runtime_error>([&] { (void)arguments.Materialize(state, runtime, snapshot); });
    Reject<std::invalid_argument>([&] { (void)ReadResourceRegisters(plan, std::array{128u}); });
    Reject<std::invalid_argument>([&] { (void)ReadResourceRegisters(plan, std::array{7u, 7u}); });
    Reject<std::runtime_error>([&] { NativeComputeArguments invalid(plan, layout, {}, shaderRegisters); });
    plan.stage = IrShaderStage::Pixel;
    Reject<std::invalid_argument>([&] { NativeComputeArguments invalid(plan, layout, roots, shaderRegisters); });
    plan.stage = IrShaderStage::Compute;
    plan.userDataBase = 2;
    Reject<std::invalid_argument>([&] { (void)ReadResourceRegisters(plan, {}); });
    plan.userDataBase = 0;
    plan.srtPlanComplete = false;
    Reject<std::invalid_argument>([&] { (void)ReadResourceRegisters(plan, {}); });
}
