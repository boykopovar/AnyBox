#include "NativeRootFixture.hpp"
#include <anybox/shader/SerializedRootArguments.hpp>

void CheckSerializedGraphicsRoots() {
    using namespace NativeRootTests;
    using namespace Fixtures;
    using namespace AnyBox::Shader;
    const std::array<NativeResourceBinding, 3> resources{{
        {NativeResourceKind::ShaderResource, NativeRootKind::Table, 0, 110, 4, 8, 4, 0},
        {NativeResourceKind::ShaderResource, NativeRootKind::Descriptor, 0, 108, 1, 4, 8, 1},
        {NativeResourceKind::ConstantBuffer, NativeRootKind::Constants, 0, 109, 0, 0, 12, 2}
    }};
    for (auto visibility : {1u, 5u}) {
        Fixture fixture;
        for (auto& parameter : fixture.root.parameters) parameter.visibility = visibility;
        auto roots = BuildSerializedStageRoots(resources, fixture.root, visibility);
        NativeRootArgumentWriter writer(fixture.root, roots, fixture.registers, NativeRootBindingMode::SerializedSlots, visibility);
        std::array<std::uint32_t, 6> output{};
        writer.Write(fixture.State(), output);
        Require(output == std::array{0x12340u, 0x98765432u, 0x12u, 99u, 100u, 101u});

        auto other = visibility == 1 ? 5u : 1u;
        Reject<std::invalid_argument>([&] { (void)BuildSerializedStageRoots(resources, fixture.root, other); });
        Reject<std::invalid_argument>([&] { (void)BuildSerializedComputeRoots(resources, fixture.root); });
        Reject<std::invalid_argument>([&] { NativeRootArgumentWriter invalid(fixture.root, roots, fixture.registers, NativeRootBindingMode::SerializedSlots, other); });
        Reject<std::invalid_argument>([&] { NativeRootArgumentWriter invalid(fixture.root, roots, fixture.registers, NativeRootBindingMode::SerializedSlots); });
        fixture.root.flags = visibility == 1 ? 2u : 32u;
        Reject<std::invalid_argument>([&] { (void)BuildSerializedStageRoots(resources, fixture.root, visibility); });
        Reject<std::invalid_argument>([&] { NativeRootArgumentWriter invalid(fixture.root, roots, fixture.registers, NativeRootBindingMode::SerializedSlots, visibility); });
        fixture.root.flags = other == 1 ? 2u : 32u;
        Require(BuildSerializedStageRoots(resources, fixture.root, visibility) == roots);
        fixture.root.flags = 0;
        for (auto& parameter : fixture.root.parameters) parameter.visibility = 0;
        Require(BuildSerializedStageRoots(resources, fixture.root, visibility) == roots);
        NativeRootArgumentWriter shared(fixture.root, roots, fixture.registers, NativeRootBindingMode::SerializedSlots, visibility);
        shared.Write(fixture.State(), output);
        Require(output == std::array{0x12340u, 0x98765432u, 0x12u, 99u, 100u, 101u});
    }
    Fixture fixture;
    for (auto visibility : {2u, 3u, 4u, 6u, 7u, UINT32_MAX}) {
        Reject<std::runtime_error>([&] { (void)BuildSerializedStageRoots(resources, fixture.root, visibility); });
        Reject<std::runtime_error>([&] { NativeRootArgumentWriter invalid(fixture.root, fixture.bindings, fixture.registers, NativeRootBindingMode::SerializedSlots, visibility); });
    }
}
