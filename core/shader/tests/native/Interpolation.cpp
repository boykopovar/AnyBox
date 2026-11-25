#include <anybox/shader/NativePixel.hpp>
#include "../buffer/Target.hpp"
#include "NativeFixtures.hpp"

void CheckSpirvValidation(std::span<const std::uint32_t> code);

namespace {
bool Contains(std::span<const std::uint32_t> code, spv::Op opcode, std::uint32_t operand) {
    for (std::size_t offset = 5; offset < code.size(); offset += code[offset] >> 16) {
        auto words = code[offset] >> 16;
        if ((code[offset] & 65535) != static_cast<std::uint32_t>(opcode)) continue;
        if (opcode == spv::OpCapability && words == 2 && code[offset + 1] == operand) return true;
        if (opcode == spv::OpDecorate && words >= 3 && code[offset + 2] == operand) return true;
    }
    return false;
}
}

void CheckNativeInterpolation() {
    using namespace AnyBox::Shader;
    using namespace ShaderRecompiler;
    using namespace Fixtures;
    AnyBox::Graphics::Container::NativeStageState state{AnyBox::Graphics::Container::NativeHardwareStage::Pixel, 0xC0, true, false};
    std::array<std::uint32_t, 5> words{0xC8080000, 0xC8090001, 0xF8001801, 2, 0xBF810000};
    for (auto wave : {32u, 64u}) {
        for (auto linear : {false, true}) {
            auto target = BufferTests::Target();
            target.subgroupSize = wave;
            ShaderPixelInputInfo input;
            input.inputNum = 1;
            input.targetOutputMode[0] = 1;
            auto kind = linear ? PixelInput::LinearCenter : PixelInput::PerspectiveCenter;
            input.psInputVgpr[static_cast<std::uint32_t>(kind)] = 0;
            NativeFrontend frontend(std::as_bytes(std::span(words)), wave);
            auto program = [&] {
                try { return LowerNativePixel(frontend, state, input, 0, target); }
                catch (const std::exception& error) { throw std::runtime_error("Pixel interpolation wave=" + std::to_string(wave) + " linear=" + std::to_string(linear) + ": " + error.what()); }
            }();
            for (const auto& block : program.Blocks()) {
                for (const auto* instruction : block->Instructions()) Require(instruction->Opcode() != IrOpcode::InterpolateHostP1 && instruction->Opcode() != IrOpcode::InterpolateHostP2);
            }
            auto compiled = CompilePixel(program, input, target, {0, 0, 0, 128});
            CheckSpirvValidation(compiled.code);
            Require(Contains(compiled.code, spv::OpDecorate, spv::DecorationNoPerspective) == linear);
            Require(!Contains(compiled.code, spv::OpCapability, spv::CapabilityFragmentBarycentricKHR));
        }
    }

    words[1] = 0xBF800000;
    auto target = BufferTests::Target();
    ShaderPixelInputInfo input;
    input.inputNum = 1;
    input.targetOutputMode[0] = 1;
    input.psInputVgpr[static_cast<std::uint32_t>(PixelInput::PerspectiveCenter)] = 0;
    NativeFrontend partial(std::as_bytes(std::span(words)), 32);
    Reject<std::runtime_error>([&] { (void)LowerNativePixel(partial, state, input, 0, target); });
    std::vector<std::uint32_t> capabilities(target.supportedCapabilities.begin(), target.supportedCapabilities.end());
    capabilities.push_back(spv::CapabilityFragmentBarycentricKHR);
    std::vector<std::string_view> extensions(target.supportedExtensions.begin(), target.supportedExtensions.end());
    extensions.push_back("SPV_KHR_fragment_shader_barycentric");
    target.supportedCapabilities = capabilities;
    target.supportedExtensions = extensions;
    auto program = LowerNativePixel(partial, state, input, 0, target);
    auto compiled = CompilePixel(program, input, target, {0, 0, 0, 128});
    CheckSpirvValidation(compiled.code);
    Require(Contains(compiled.code, spv::OpCapability, spv::CapabilityFragmentBarycentricKHR));
}
