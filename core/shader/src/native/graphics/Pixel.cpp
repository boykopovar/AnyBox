#include "Stages.hpp"
#include <anybox/shader/NativePixel.hpp>

namespace {
std::uint8_t OutputMode(std::uint32_t format) {
    switch (format) {
        case 2: case 6: case 10: case 11: case 13: case 16: case 24: case 26: case 28: case 29: case 31: case 34: case 35: case 37: case 41: case 49: case 51: case 54: case 56: case 58: case 61: case 63: case 65: case 67: case 87: case 88: case 91: case 93: return 4;
        default: throw std::runtime_error("Native graphics render target output mode is not implemented for this format");
    }
}
}

AnyBox::Shader::GraphicsArtifact AnyBox::Shader::NativeGraphics::CompilePixel(std::span<const std::byte> program, const Graphics::Container::NativeVertexOutputs& vertex, const Graphics::Container::RootSignatureLayout& root, const GraphicsShaderLayout& layout, const GraphicsArtifactKey& key, const ShaderRecompiler::SpirvTargetOptions& target) {
    using namespace ShaderRecompiler;
    auto bindings = Graphics::Container::ReadNativeBindings(program);
    auto buffers = ReadNativeDirectBufferBindings(bindings, root, 5);
    RootArgumentPlan plan(buffers, root, 0, 5);
    auto inputs = Graphics::Container::ReadNativePixelInputs(program);
    ShaderPixelInputInfo input;
    LinkNativePixelAttributes(input, inputs, vertex);
    for (std::uint32_t index = 0; index < layout.renderTargetCount; ++index) input.targetOutputMode[index] = OutputMode(layout.renderTargetFormats[index]);
    for (std::uint32_t index = 0; index < 16; ++index) if ((inputs.inputEnable & (1u << index)) != 0) input.psInputVgpr[index] = PixelInputVgpr(inputs.inputEnable, static_cast<PixelInput>(index));
    auto enabled = [&](PixelInput field) { return (inputs.inputEnable & PixelInputBit(field)) != 0; };
    input.psPosX = enabled(PixelInput::PositionX);
    input.psPosY = enabled(PixelInput::PositionY);
    input.psPosZ = enabled(PixelInput::PositionZ);
    input.psPosW = enabled(PixelInput::PositionW);
    input.psFrontFace = enabled(PixelInput::FrontFace);
    input.psAncillary = enabled(PixelInput::Ancillary);
    if (enabled(PixelInput::SampleCoverage) || enabled(PixelInput::PositionFixedPoint) || enabled(PixelInput::LineStipple)) throw std::runtime_error("Native graphics coverage, fixed point position and line stipple inputs are not implemented");
    NativeFrontend frontend(program);
    auto translated = LowerNativePixel(frontend, Graphics::Container::ReadNativeStageState(program), input, UserRegisterCount(bindings), target);
    BindNativeGraphicsRootArguments(translated, bindings.roots, plan);
    for (const auto& output : translated.Metadata().exportInfo) if (output.kind == ExportTargetKind::MrtZ) throw std::runtime_error("Native graphics depth, stencil and coverage export configuration is not implemented");
    auto compiled = AnyBox::Shader::CompilePixel(translated, input, target, {0, 0, 0, 128});
    return MakeGraphicsArtifact(std::move(compiled), key, frontend.GetWaveSize(), target.subgroupSize, buffers);
}
