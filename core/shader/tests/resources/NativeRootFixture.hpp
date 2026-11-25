#pragma once

#include "Fixtures.hpp"
#include <anybox/shader/NativeRootArgumentWriter.hpp>
#include <array>

namespace NativeRootTests {
using namespace AnyBox::Graphics::Container;
using AnyBox::Shader::NativeRootArgumentWriter;

struct Fixture {
    RootSignatureLayout root{2, 0, 0, {}};
    std::vector<NativeRootBinding> bindings;
    std::vector<std::uint32_t> registers{4, 8, 9, 12, 13, 14};

    Fixture() {
        root.parameters = {
            {RootParameterKind::Table, 0, 0, 0, 0, 0, {{RootRangeKind::ShaderResource, 1, 10, 1, 1, UINT32_MAX}, {RootRangeKind::UnorderedAccess, 1, 7, 1, 1, UINT32_MAX}, {RootRangeKind::ConstantBuffer, 1, 4, 1, 1, UINT32_MAX}}},
            {RootParameterKind::ShaderResource, 0, 8, 2, 0, 0, {}},
            {RootParameterKind::Constants, 0, 9, 3, 3, 0, {}}
        };
        bindings = {
            {NativeRootKind::Table, 0, 1, 10, 4, 0},
            {NativeRootKind::Table, 8, 1, 7, 4, 0},
            {NativeRootKind::Table, 16, 1, 4, 4, 0},
            {NativeRootKind::Descriptor, 0, 2, 8, 8, 1},
            {NativeRootKind::Constants, 0, 3, 9, 12, 2},
            {NativeRootKind::ImmediateSampler, 0, 0, 5, 0, 127}
        };
    }

    AnyBox::Graphics::Commands::RootState State() const {
        AnyBox::Graphics::Commands::RootState state(root);
        state.Write(0, 0, std::array{0x12340u});
        state.Write(1, 0, std::array{0x98765432u, 0x12u});
        state.Write(2, 0, std::array{99u, 100u, 101u});
        return state;
    }
};
}
