#include <anybox/shader/NativeShaderArguments.hpp>
#include "../native/RootFixture.hpp"
#include <Optimization/ResourceMaterializer.hpp>
#include <algorithm>
#include <source_location>

namespace {
void CheckArgument(bool value, const std::source_location& location = std::source_location::current()) {
    if (!value) throw std::runtime_error("Native graphics arguments test failed at line " + std::to_string(location.line()));
}

struct Memory {
    std::uint32_t reads = 0;
    static bool Read(void* context, std::uint64_t address, std::uint32_t* output) {
        if (address != 0x10000 || !output) return false;
        ++static_cast<Memory*>(context)->reads;
        *output = 0x12345678;
        return true;
    }
};
}

void CheckNativeGraphicsArguments() {
    using namespace AnyBox::Shader;
    using namespace AnyBox::Graphics::Container;
    using namespace ShaderRecompiler;
    using namespace Fixtures;
    NativeTests::RootFixture fixture(32, 16);
    auto translation = fixture.Translate();
    (void)CompileCompute(translation.program, translation.input, BufferTests::Target(), {0, 0, 0, 128});
    auto original = ResourceMaterializer{}.ExtractPlan(translation.program);
    auto encoded = EncodeResourcePlan(original, {});
    for (auto stage : {IrShaderStage::Vertex, IrShaderStage::Pixel}) {
        auto plan = DecodeResourcePlan(encoded, {}, IrShaderStage::Compute);
        plan.stage = stage;
        plan = DecodeResourcePlan(EncodeResourcePlan(plan, {}), {}, stage);
        auto visibility = stage == IrShaderStage::Vertex ? 1u : 5u;
        RootSignatureLayout layout{2, 0, 0, {{RootParameterKind::Constants, visibility, 5, 0, 1, 0, {}}}};
        std::vector<NativeRootBinding> roots{{NativeRootKind::Constants, 0, 1, 5, 8, 0}};
        const std::array shaderRegisters{8u, 2u};
        NativeShaderArguments arguments(plan, layout, roots, shaderRegisters);
        CheckArgument(std::ranges::equal(arguments.GetSystemRegisters(), std::array{0u, 1u, 2u}));
        AnyBox::Graphics::Commands::RootState state(layout);
        state.Write(0, 0, std::array{777u});
        std::array<std::uint32_t, 8> system{0x10000, 0, 23};
        Reject<std::invalid_argument>([&] { (void)arguments.ReadUserData(state); });
        Reject<std::invalid_argument>([&] { (void)arguments.ReadUserData(state, std::span(system).first(2)); });
        auto data = arguments.ReadUserData(state, std::span(system).first(3));
        CheckArgument(data[0] == 0x10000 && data[1] == 0 && data[2] == 23 && data[8] == 777);
        CheckArgument(std::ranges::count(data, 0u) == 125);
        Memory memory;
        SrtRuntime runtime;
        runtime.readMemory = &Memory::Read;
        runtime.userContext = &memory;
        ResourceSnapshot snapshot;
        CheckArgument(arguments.Materialize(state, runtime, snapshot, system) == data);
        CheckArgument(memory.reads != 0 && snapshot.userData.size() == plan.userDataCount && snapshot.userData[0] == 0x10000 && snapshot.userData[1] == 0);
        CheckArgument(std::ranges::find(snapshot.flattenedSrt, 0x12345678u) != snapshot.flattenedSrt.end());
        system[0] = 0x20000;
        Reject<std::runtime_error>([&] { (void)arguments.Materialize(state, runtime, snapshot, system); });
        system[0] = 0x10000;
        roots.push_back({NativeRootKind::Constants, 0, 1, 5, 2, 0});
        NativeShaderArguments mapped(plan, layout, roots, shaderRegisters);
        CheckArgument(std::ranges::equal(mapped.GetSystemRegisters(), std::array{0u, 1u}));
        CheckArgument(mapped.ReadUserData(state, std::span(system).first(2))[2] == 777);
        Reject<std::runtime_error>([&] { NativeShaderArguments invalid(plan, layout, roots, std::array{9u}); });
        auto hidden = layout;
        hidden.parameters.front().visibility = visibility == 1 ? 5u : 1u;
        Reject<std::invalid_argument>([&] { NativeShaderArguments invalid(plan, hidden, roots, shaderRegisters); });
        Reject<std::invalid_argument>([&] { (void)arguments.ReadUserData(state, std::array<std::uint32_t, 9>{}); });
    }
}
