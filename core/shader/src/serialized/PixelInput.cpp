#include <anybox/shader/SerializedGraphicsCompiler.hpp>
#include <anybox/graphics/container/SerializedPixelInterface.hpp>
#include <anybox/graphics/container/SerializedGraphicsState.hpp>
#include <algorithm>

ShaderRecompiler::ShaderPixelInputInfo AnyBox::Shader::ReadSerializedPixelInput(std::span<const std::byte> state, std::span<const std::byte> auxiliary) {
    using namespace ShaderRecompiler;
    auto fixed = Graphics::Container::ReadSerializedGraphicsFixedState(state, auxiliary);
    auto pixel = Graphics::Container::ReadSerializedPixelInterface(state);
    if (fixed.samples != 1 || fixed.sampleQuality != 0) throw std::runtime_error("Serialized pixel multisample locations and invocation modes are not implemented");
    if (fixed.alphaToCoverage != 0) throw std::runtime_error("Serialized pixel alpha coverage modes are not implemented");
    if ((pixel.depthControl & ~0x40u) != 0) throw std::runtime_error("Serialized pixel depth, stencil, coverage exports and ordering modes are not implemented");
    if ((pixel.barycentricControl & ~0x1111u) != 0) throw std::runtime_error("Serialized pixel position and extended barycentric controls are not implemented");
    constexpr std::uint32_t unsupported = PixelInputBit(PixelInput::PerspectivePullModel) | PixelInputBit(PixelInput::LineStipple) | PixelInputBit(PixelInput::PositionX) | PixelInputBit(PixelInput::PositionY) | PixelInputBit(PixelInput::PositionZ) | PixelInputBit(PixelInput::PositionW) | PixelInputBit(PixelInput::SampleCoverage) | PixelInputBit(PixelInput::PositionFixedPoint);
    if ((pixel.inputAddress & unsupported) != 0) throw std::runtime_error("Serialized pixel pull-model, position, coverage and stipple inputs are not implemented");
    ShaderPixelInputInfo result;
    result.inputNum = pixel.inputCount;
    std::ranges::copy(pixel.interpolators, result.interpolatorSettings);
    for (std::uint32_t index = 0; index < pixel.inputCount; ++index) {
        if ((pixel.interpolators[index] & ~0x73Fu) != 0) throw std::runtime_error("Serialized pixel packed, duplicated and sprite interpolation is not implemented");
    }
    for (std::uint32_t index = 0; index < 16; ++index) {
        if ((pixel.inputAddress & (1u << index)) != 0) result.psInputVgpr[index] = PixelInputVgpr(pixel.inputAddress, static_cast<PixelInput>(index));
    }
    for (std::uint32_t index = 0; index < 8; ++index) {
        auto mode = (pixel.colorFormat >> (index * 4)) & 15;
        if (mode > 9) throw std::invalid_argument("Serialized pixel color export format is reserved");
        result.targetOutputMode[index] = static_cast<std::uint8_t>(mode);
    }
    result.psFrontFace = (pixel.inputAddress & PixelInputBit(PixelInput::FrontFace)) != 0;
    result.psAncillary = (pixel.inputAddress & PixelInputBit(PixelInput::Ancillary)) != 0;
    result.psNoPerspective = (pixel.inputAddress & PixelInputBit(PixelInput::LinearCenter)) != 0;
    result.psPixelKillEnable = (pixel.depthControl & 0x40) != 0;
    return result;
}

AnyBox::Shader::SerializedGraphicsStageCompilation AnyBox::Shader::CompileSerializedPixel(const SerializedGraphicsStageSource& source, std::span<const std::byte> auxiliary, const Graphics::Container::RootSignatureLayout& root, const ShaderRecompiler::SpirvTargetOptions& target) {
    return CompileSerializedPixel(source, root, ReadSerializedPixelInput(source.state, auxiliary), target);
}
