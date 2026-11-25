#include "NativeRootFixture.hpp"
#include <algorithm>

void CheckNativeRootWriterFailures();

void CheckNativeRootWriter() {
    using namespace NativeRootTests;
    using namespace Fixtures;
    Fixture fixture;
    NativeRootArgumentWriter writer(fixture.root, fixture.bindings, fixture.registers);
    auto state = fixture.State();
    std::array<std::uint32_t, 8> guarded{};
    guarded.front() = guarded.back() = 0xAABBCCDD;
    writer.Write(state, std::span(guarded).subspan(1, 6));
    const std::array expected{0xAABBCCDDu, 0x12340u, 0x98765432u, 0x12u, 99u, 100u, 101u, 0xAABBCCDDu};
    Require(guarded == expected && writer.GetWordCount() == 6);
    std::array<std::uint32_t, 4> registers{14, 4, 9, 12};
    NativeRootArgumentWriter selected(fixture.root, fixture.bindings, registers);
    std::array<std::uint32_t, 4> output{};
    selected.Write(state, output);
    Require(output == std::array{101u, 0x12340u, 0x12u, 99u});
    auto legacy = fixture.root;
    legacy.version = 1;
    NativeRootArgumentWriter legacyWriter(legacy, fixture.bindings, registers);
    legacyWriter.Write(state, output);
    Require(output == std::array{101u, 0x12340u, 0x12u, 99u});
    for (auto kind : {RootParameterKind::ConstantBuffer, RootParameterKind::ShaderResource, RootParameterKind::UnorderedAccess}) {
        auto root = fixture.root;
        root.parameters[1].kind = kind;
        NativeRootArgumentWriter descriptor(root, fixture.bindings, std::array{8u, 9u});
        AnyBox::Graphics::Commands::RootState descriptorState(root);
        descriptorState.Write(1, 0, std::array{0xFFFF0000u, 0x123u});
        std::array<std::uint32_t, 2> address{};
        descriptor.Write(descriptorState, address);
        Require(address == std::array{0xFFFF0000u, 0x123u});
    }
    auto sampler = fixture.root;
    sampler.parameters[0].ranges = {{RootRangeKind::Sampler, 8, 10, 0, 1, 0}};
    std::array<NativeRootBinding, 1> binding{{{NativeRootKind::Table, 20, 1, 15, 4, 0}}};
    NativeRootArgumentWriter samplerWriter(sampler, binding, std::array{4u});
    AnyBox::Graphics::Commands::RootState samplerState(sampler);
    samplerState.Write(0, 0, std::array{0x10000u});
    std::array<std::uint32_t, 1> pointer{};
    samplerWriter.Write(samplerState, pointer);
    Require(pointer.front() == 0x10000);
    NativeRootArgumentWriter empty(fixture.root, fixture.bindings, {});
    Require(empty.GetWordCount() == 0);
    empty.Write(state, {});
    CheckNativeRootWriterFailures();
}
