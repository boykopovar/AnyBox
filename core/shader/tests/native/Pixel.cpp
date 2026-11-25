#include <anybox/shader/NativePixel.hpp>
#include <anybox/shader/NativeLinkage.hpp>
#include "../buffer/Target.hpp"
#include "NativeFixtures.hpp"
#include <array>

void CheckSpirvValidation(std::span<const std::uint32_t> code);

namespace {
std::vector<std::byte> PixelProgram(bool wide) {
    auto data = Fixtures::MakeComputeProgram();
    data.resize(0x1C0);
    Fixtures::Set(data, 0x100, 5);
    Fixtures::Set(data, 0xC8, 0x80);
    Fixtures::Set(data, 0xD4, wide ? 0 : 0x20);
    for (std::size_t offset = 0x168; offset < data.size(); offset += 4) Fixtures::Set(data, offset, 0xBF9F0000);
    constexpr std::array<std::uint32_t, 7> instructions{0x7E0002FF, 0x3C003C00, 0x7E0202FF, 0x3C003C00, 0xF8001C0F, 0x00000100, 0xBF810000};
    for (std::size_t index = 0; index < instructions.size(); ++index) Fixtures::Set(data, 0x168 + index * 4, instructions[index]);
    return data;
}

void CheckLinkedPixel() {
    using namespace AnyBox::Shader;
    using namespace AnyBox::Graphics::Container;
    auto data = PixelProgram(false);
    for (std::size_t offset = 0x168; offset < data.size(); offset += 4) Fixtures::Set(data, offset, 0xBF9F0000);
    constexpr std::array<std::uint32_t, 4> instructions{0xC8020002, 0xF8001801, 0, 0xBF810000};
    for (std::size_t index = 0; index < instructions.size(); ++index) Fixtures::Set(data, 0x168 + index * 4, instructions[index]);
    NativePixelInputs pixel;
    pixel.count = 1;
    pixel.attributes[0] = {6, 7, 1, 1};
    NativeVertexOutputs vertex;
    vertex.count = 3;
    vertex.parameters = {{{6, 2}, {6, 4}, {6, 7}}};
    ShaderRecompiler::ShaderPixelInputInfo input;
    input.targetOutputMode[0] = 1;
    LinkNativePixelAttributes(input, pixel, vertex);
    auto compiled = CompileNativePixel(data, input, 0, BufferTests::Target(), {0, 0, 0, 128});
    CheckSpirvValidation(compiled.code);
    bool flat = false;
    bool location = false;
    for (std::size_t offset = 5; offset < compiled.code.size();) {
        auto count = compiled.code[offset] >> 16;
        auto opcode = compiled.code[offset] & 65535;
        Fixtures::Require(count != 0 && count <= compiled.code.size() - offset);
        if (opcode == 71 && count >= 3 && compiled.code[offset + 2] == 14) flat = true;
        if (opcode == 71 && count >= 4 && compiled.code[offset + 2] == 30 && compiled.code[offset + 3] == 2) location = true;
        offset += count;
    }
    Fixtures::Require(flat && location);
}
}

void CheckNativePixel() {
    CheckLinkedPixel();
    using namespace AnyBox::Shader;
    using namespace ShaderRecompiler;
    using namespace Fixtures;
    for (bool wide : {false, true}) {
        auto data = PixelProgram(wide);
        auto target = BufferTests::Target();
        target.subgroupSize = wide ? 64 : 32;
        ShaderPixelInputInfo input;
        input.targetOutputMode[0] = 4;
        auto compiled = CompileNativePixel(data, input, 0, target, {0, 0, 0, 128});
        CheckSpirvValidation(compiled.code);
        bool fragment = false;
        bool output = false;
        for (std::size_t offset = 5; offset < compiled.code.size();) {
            auto count = compiled.code[offset] >> 16;
            auto opcode = compiled.code[offset] & 65535;
            Require(count != 0 && count <= compiled.code.size() - offset);
            if (opcode == 15) fragment = compiled.code[offset + 1] == 4;
            if (opcode == 71 && count >= 4 && compiled.code[offset + 2] == 30 && compiled.code[offset + 3] == 0) output = true;
            offset += count;
        }
        Require(fragment && output);
        auto state = AnyBox::Graphics::Container::ReadNativeStageState(data);
        NativeFrontend frontend(data);
        auto lowered = LowerNativePixel(frontend, state, input, 0, target);
        Require(lowered.Resources().stage == IrShaderStage::Pixel && lowered.WaveSize() == target.subgroupSize);
        auto incompatible = target;
        incompatible.subgroupSize = wide ? 32 : 64;
        auto adjusted = CompilePixel(lowered, input, incompatible, {0, 0, 0, 128});
        CheckSpirvValidation(adjusted.code);
        state.stage = AnyBox::Graphics::Container::NativeHardwareStage::Compute;
        Reject<std::invalid_argument>([&] { LowerNativePixel(frontend, state, input, 0, target); });
        state = AnyBox::Graphics::Container::ReadNativeStageState(data);
        state.floatControl = 256;
        Reject<std::invalid_argument>([&] { LowerNativePixel(frontend, state, input, 0, target); });
        state.floatControl = 0xC0;
        input.inputNum = 33;
        Reject<std::invalid_argument>([&] { LowerNativePixel(frontend, state, input, 0, target); });
        input.inputNum = 0;
        target.subgroupSize = 16;
        Reject<std::invalid_argument>([&] { LowerNativePixel(frontend, state, input, 0, target); });
    }
    auto data = PixelProgram(false);
    Set(data, 0x100, 6);
    ShaderPixelInputInfo input;
    input.targetOutputMode[0] = 4;
    Reject<std::invalid_argument>([&] { CompileNativePixel(data, input, 0, BufferTests::Target(), {0, 0, 0, 128}); });
}
