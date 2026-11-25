#include "NativeRootFixture.hpp"

void CheckNativeRootWriterFailures() {
    using namespace NativeRootTests;
    using namespace Fixtures;
    Fixture fixture;
    auto rejectBinding = [&](auto modify) {
        auto bindings = fixture.bindings;
        modify(bindings);
        Reject<std::exception>([&] { NativeRootArgumentWriter rejected(fixture.root, bindings, fixture.registers); });
    };
    rejectBinding([](auto& bindings) { bindings[0].registerCount = 0; });
    rejectBinding([](auto& bindings) { bindings[0].registerCount = 2; });
    rejectBinding([](auto& bindings) { bindings[0].rootParameter = 99; });
    rejectBinding([](auto& bindings) { bindings[0].tableOffset = 1; });
    rejectBinding([](auto& bindings) { bindings[0].shaderRegister = 999; });
    rejectBinding([](auto& bindings) { bindings[0].firstUserRegister = 128; });
    rejectBinding([](auto& bindings) { bindings[3].registerCount = 1; });
    rejectBinding([](auto& bindings) { bindings[3].firstUserRegister = 127; });
    rejectBinding([](auto& bindings) { bindings[3].tableOffset = 1; });
    rejectBinding([](auto& bindings) { bindings[3].shaderRegister = 9; });
    rejectBinding([](auto& bindings) { bindings[4].registerCount = 2; });
    rejectBinding([](auto& bindings) { bindings[4].firstUserRegister = 4; });
    rejectBinding([](auto& bindings) { bindings.push_back(bindings[3]); });
    rejectBinding([](auto& bindings) { bindings[5].kind = NativeRootKind::ResourceHeap; });
    for (auto registers : {std::array{4u, 4u}, std::array{4u, 128u}, std::array{0u, 1u}}) Reject<std::exception>([&] { NativeRootArgumentWriter rejected(fixture.root, fixture.bindings, registers); });
    auto rejectRoot = [&](auto modify) {
        auto root = fixture.root;
        modify(root);
        Reject<std::exception>([&] { NativeRootArgumentWriter rejected(root, fixture.bindings, fixture.registers); });
    };
    rejectRoot([](auto& root) { root.version = 3; });
    rejectRoot([](auto& root) { root.flags = 0x80; });
    rejectRoot([](auto& root) { root.flags = 0x40000000; });
    rejectRoot([](auto& root) { root.parameters[0].visibility = 1; });
    rejectRoot([](auto& root) { root.parameters[0].kind = RootParameterKind::Constants; });
    rejectRoot([](auto& root) { root.parameters[0].ranges[0].count = 0; });
    rejectRoot([](auto& root) { root.parameters[0].ranges[0].flags = 0x80000000; });
    rejectRoot([](auto& root) { root.parameters[0].ranges[0].count = UINT32_MAX; });
    rejectRoot([](auto& root) { root.parameters[0].ranges[0].firstRegister = UINT32_MAX; root.parameters[0].ranges[0].count = 2; });
    NativeRootArgumentWriter writer(fixture.root, fixture.bindings, fixture.registers);
    auto state = fixture.State();
    std::array<std::uint32_t, 6> destination{};
    Reject<std::invalid_argument>([&] { writer.Write(state, std::span(destination).first(5)); });
    AnyBox::Graphics::Commands::RootState uninitialized(fixture.root);
    Reject<std::logic_error>([&] { writer.Write(uninitialized, destination); });
    for (auto address : {0u, 1u, 0x10004u}) {
        auto invalid = fixture.State();
        invalid.Write(0, 0, std::array{address});
        Reject<std::invalid_argument>([&] { writer.Write(invalid, destination); });
    }
    auto wrongRoot = fixture.root;
    wrongRoot.parameters[2].wordCount = 4;
    AnyBox::Graphics::Commands::RootState wrongState(wrongRoot);
    wrongState.Write(0, 0, std::array{0x10000u});
    wrongState.Write(1, 0, std::array{0u, 0u});
    Reject<std::invalid_argument>([&] { writer.Write(wrongState, destination); });
}
