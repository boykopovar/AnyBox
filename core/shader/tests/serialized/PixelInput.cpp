#include "Fixture.hpp"
#include "../buffer/Target.hpp"

void CheckSpirvValidation(std::span<const std::uint32_t> code);

void CheckSerializedPixelInput() {
    using namespace AnyBox::Shader;
    using namespace ShaderRecompiler;
    using namespace Fixtures;
    SerializedGraphicsTests::StageFixture fixture(true);
    auto read = [&] { return ReadSerializedPixelInput(fixture.graphics.state, fixture.graphics.auxiliary); };
    auto input = read();
    Require(input.inputNum == 2 && input.InputSlot(0) == 0 && input.InputSlot(1) == 1 && input.targetOutputMode[0] == 4);
    for (std::uint32_t index = 0; index < 16; ++index) Require(input.psInputVgpr[index] == ShaderPixelInputInfo::NoPixelInputVgpr);
    constexpr std::uint32_t unsupported = 0xCF88;
    for (std::uint32_t index = 0; index < 16; ++index) {
        fixture.graphics.Context(0, 1u << index);
        if ((unsupported & (1u << index)) != 0) Reject<std::runtime_error>(read);
        else {
            input = read();
            Require(input.psInputVgpr[index] == 0);
            Require(input.psFrontFace == (index == 12) && input.psAncillary == (index == 13));
            Require(input.psNoPerspective == (index == 5));
        }
    }
    fixture.graphics.Context(0, 2 | 32 | 4096 | 8192);
    input = read();
    Require(input.psInputVgpr[1] == 0 && input.psInputVgpr[5] == 2 && input.psInputVgpr[12] == 4 && input.psInputVgpr[13] == 5);
    fixture.graphics.Context(0, 0);
    fixture.graphics.Context(8, 64);
    Require(read().psPixelKillEnable);
    fixture.graphics.Context(8, 1);
    Reject<std::runtime_error>(read);
    fixture.graphics.Context(8, 0);
    fixture.graphics.Context(16, 0x10000);
    Reject<std::runtime_error>(read);
    fixture.graphics.Context(16, 0x1111);
    Require(read().inputNum == 2);
    fixture.graphics.auxiliary[40] = std::byte{2};
    Reject<std::runtime_error>(read);
    fixture.graphics.auxiliary[40] = std::byte{1};
    fixture.graphics.Context(112, 1);
    Reject<std::runtime_error>(read);
    fixture.graphics.Context(112, 0);
    Set(fixture.graphics.state, 368, 0x80000);
    Reject<std::runtime_error>(read);
    Set(fixture.graphics.state, 368, 0x523);
    Require(read().interpolatorSettings[0] == 0x523);
    Set(fixture.graphics.state, 368, 0);
    fixture.graphics.Context(24, 10);
    Reject<std::invalid_argument>(read);
    for (std::uint32_t mode = 0; mode < 10; ++mode) {
        fixture.graphics.Context(24, mode * 0x11111111u);
        auto configured = read();
        for (auto value : configured.targetOutputMode) Require(value == mode);
    }
    fixture.graphics.Context(24, 4);
    AnyBox::Graphics::Container::RootSignatureLayout root{2, 0, 0, {}};
    auto compiled = CompileSerializedPixel(fixture.Source(64), fixture.graphics.auxiliary, root, BufferTests::Target());
    CheckSpirvValidation(compiled.shader.code);
}
