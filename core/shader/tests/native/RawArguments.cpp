#include "RootFixture.hpp"
#include <anybox/shader/NativeRootArgumentWriter.hpp>

void CheckSpirvValidation(std::span<const std::uint32_t> code);

void CheckNativeRawRootArguments() {
    using namespace AnyBox::Shader;
    using namespace AnyBox::Graphics::Container;
    using namespace Fixtures;
    for (unsigned wave : {32u, 64u}) {
        for (unsigned slot : {0u, 2u}) {
            NativeTests::RootFixture fixture(wave, slot);
            auto translation = fixture.Translate();
            BindNativeRawRootArguments(translation, fixture.roots, fixture.root);
            Reject<std::logic_error>([&] { BindNativeRawRootArguments(translation, fixture.roots, fixture.root); });
            auto compiled = CompileCompute(translation.program, translation.input, BufferTests::Target(), {0, 0, 0, 128});
            Require(compiled.bindings.layout.userDataRegisters == std::vector<std::uint32_t>{slot});
            CheckSpirvValidation(compiled.code);
        }
    }
    auto data = MakeComputeProgram();
    data.resize(0x1C0);
    Set(data, 0xC8, 0x80);
    Set(data, 0x10C, 0);
    Set(data, 0x138, 2);
    Set(data, 0x13C, 1);
    Set(data, 0x140, 1);
    Set(data, 0x144, 1);
    const std::array code{0xBE880300u, 0xBE890380u, 0xF4000284u, 0xFA000000u, 0xBF8C0000u, 0x7E06020Au, 0x7E040282u, 0x34020082u, 0xDC708000u, 0x007F0301u, 0xBF810000u};
    for (std::size_t offset = 0x168; offset < data.size(); offset += 4) Set(data, offset, 0xBF9F0000);
    for (std::size_t index = 0; index < code.size(); ++index) Set(data, 0x168 + index * 4, code[index]);
    RootSignatureLayout root{2, 0, 0, {{RootParameterKind::Table, 0, 0, 0, 0, 0, {{RootRangeKind::ConstantBuffer, 1, 1, 0, 0, 0}}}}};
    const std::array<NativeRootBinding, 1> roots{{{NativeRootKind::Table, 0, 1, 1, 0, 0}}};
    NativeFrontend frontend(data);
    auto state = ReadNativeComputeState(data);
    auto target = BufferTests::Target();
    auto translation = LowerNativeCompute(frontend, state, ShaderRecompiler::ShaderFloatMode{state.floatControl, state.dx10Clamp, state.ieeeMode, false}, target);
    BindNativeRawRootArguments(translation, roots, root);
    auto compiled = CompileCompute(translation.program, translation.input, target, {0, 0, 0, 128});
    Require(compiled.bindings.layout.userDataRegisters == std::vector<std::uint32_t>{0});
    bool memory = false;
    for (const auto& descriptor : compiled.bindings.layout.descriptors) {
        Require(static_cast<std::uint32_t>(descriptor.kind) != 61);
        memory |= static_cast<std::uint32_t>(descriptor.kind) == 59;
    }
    Require(memory);
    CheckSpirvValidation(compiled.code);
    auto invalid = LowerNativeCompute(frontend, state, ShaderRecompiler::ShaderFloatMode{state.floatControl, state.dx10Clamp, state.ieeeMode, false}, target);
    auto wrongRoots = roots;
    wrongRoots.front().firstUserRegister = 1;
    Reject<std::exception>([&] { BindNativeRawRootArguments(invalid, wrongRoots, root); });
}
