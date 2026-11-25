#include "NativeRootFixture.hpp"
#include <anybox/shader/SerializedRootArguments.hpp>

void CheckSerializedComputeRoots() {
    using namespace NativeRootTests;
    using namespace Fixtures;
    using AnyBox::Shader::BuildSerializedComputeRoots;
    Fixture fixture;
    fixture.root.staticSamplerCount = 1;
    fixture.bindings.back().rootParameter = 0;
    std::vector<NativeResourceBinding> resources{
        {NativeResourceKind::ShaderResource, NativeRootKind::Table, 0, 10, 4, 8, 4, 0},
        {NativeResourceKind::UnorderedAccess, NativeRootKind::Table, 8, 7, 4, 8, 4, 0},
        {NativeResourceKind::ConstantBuffer, NativeRootKind::Table, 16, 4, 0, 4, 4, 0},
        {NativeResourceKind::ShaderResource, NativeRootKind::Descriptor, 0, 8, 1, 4, 8, 1},
        {NativeResourceKind::ConstantBuffer, NativeRootKind::Constants, 0, 9, 0, 0, 12, 2},
        {NativeResourceKind::Sampler, NativeRootKind::ImmediateSampler, 0, 5, 0, 4, 0, 0}
    };
    auto roots = BuildSerializedComputeRoots(resources, fixture.root);
    Require(roots == fixture.bindings);
    NativeRootArgumentWriter writer(fixture.root, roots, fixture.registers);
    std::array<std::uint32_t, 6> output{};
    writer.Write(fixture.State(), output);
    Require(output == std::array{0x12340u, 0x98765432u, 0x12u, 99u, 100u, 101u});
    auto reject = [&](auto modify) {
        auto invalid = resources;
        modify(invalid);
        Reject<std::exception>([&] { (void)BuildSerializedComputeRoots(invalid, fixture.root); });
    };
    reject([](auto& values) { values[0].kind = NativeResourceKind::UnorderedAccess; });
    reject([](auto& values) { values[0].descriptorWords = 2; });
    reject([](auto& values) { values[0].firstShaderRegister = 9; });
    reject([](auto& values) { values[0].rootParameter = 99; });
    reject([](auto& values) { values[0].firstUserRegister = 128; });
    reject([](auto& values) { values[1].tableOffset = 0; });
    reject([](auto& values) { values[2].descriptorWords = 8; });
    reject([](auto& values) { values[3].kind = NativeResourceKind::UnorderedAccess; });
    reject([](auto& values) { values[3].descriptorWords = 8; });
    reject([](auto& values) { values[4].kind = NativeResourceKind::ShaderResource; });
    reject([](auto& values) { values[5].kind = NativeResourceKind::ShaderResource; });
    reject([](auto& values) { values[5].rootParameter = 1; });
    reject([](auto& values) { values[5].descriptorWords = 8; });
    for (auto kind : {NativeRootKind::ResourceHeap, NativeRootKind::SamplerHeap}) reject([&](auto& values) { values[0].rootKind = kind; });
    reject([](auto& values) { values[0].kind = static_cast<NativeResourceKind>(255); });
    auto samplerRoot = fixture.root;
    samplerRoot.parameters[0].ranges = {{RootRangeKind::Sampler, 2, 5, 0, 1, 0}};
    std::array<NativeResourceBinding, 1> sampler{{{NativeResourceKind::Sampler, NativeRootKind::Table, 4, 6, 0, 4, 0, 0}}};
    auto samplerBindings = BuildSerializedComputeRoots(sampler, samplerRoot);
    Require(samplerBindings.front() == NativeRootBinding{NativeRootKind::Table, 4, 1, 6, 0, 0});
}
