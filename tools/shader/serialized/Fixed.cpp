#include "../Serialized.hpp"
#include <anybox/graphics/container/SerializedGraphicsState.hpp>
#include <anybox/graphics/container/SerializedGraphicsShader.hpp>
#include <anybox/graphics/container/SerializedPixelInterface.hpp>
#include <sstream>

std::string AnyBox::Shader::Tools::InspectSerializedGraphicsState(std::span<const std::byte> state, std::span<const std::byte> auxiliary) {
    auto result = Graphics::Container::ReadSerializedGraphicsFixedState(state, auxiliary);
    std::ostringstream output;
    for (auto stage : {Graphics::Container::NativeHardwareStage::Vertex, Graphics::Container::NativeHardwareStage::Pixel}) {
        auto shader = Graphics::Container::ReadSerializedGraphicsShaderState(state, stage);
        output << "stage=" << (stage == Graphics::Container::NativeHardwareStage::Vertex ? "vertex" : "pixel") << " user_registers=" << shader.userRegisters << " float_mode=" << shader.shader.floatControl << " dx10_clamp=" << shader.shader.dx10Clamp << " ieee_mode=" << shader.shader.ieeeMode << " wave=" << shader.waveSize << '\n';
    }
    auto pixel = Graphics::Container::ReadSerializedPixelInterface(state);
    output << "pixel_inputs=" << pixel.inputCount << " input_address=" << pixel.inputAddress << " depth_control=" << pixel.depthControl << " barycentric_control=" << pixel.barycentricControl << " color_format=" << pixel.colorFormat << '\n';
    for (std::uint32_t index = 0; index < pixel.inputCount; ++index) output << "interpolator=" << index << " settings=" << pixel.interpolators[index] << '\n';
    output << "targets=" << result.targetCount << " depth_format=" << result.depthFormat << " samples=" << result.samples << " quality=" << result.sampleQuality << " mask=" << result.sampleMask << " topology=" << result.topology << " strip_cut=" << result.stripCut << " alpha_coverage=" << result.alphaToCoverage << " independent_blend=" << result.independentBlend << '\n';
    const auto& rasterizer = result.rasterizer;
    output << "fill=" << rasterizer.fill << " cull=" << rasterizer.cull << " front_ccw=" << rasterizer.frontCounterClockwise << " depth_clip=" << rasterizer.depthClip << " depth_bias=" << rasterizer.depthBias << " clamp=" << rasterizer.depthBiasClamp << " slope=" << rasterizer.slopeScaledDepthBias << " forced_samples=" << rasterizer.forcedSampleCount << " conservative=" << rasterizer.conservative << " multisample=" << rasterizer.multisample << " antialiased_line=" << rasterizer.antialiasedLine << '\n';
    const auto& depth = result.depth;
    output << "depth=" << depth.depthEnable << " write=" << depth.depthWrite << " comparison=" << depth.comparison << " stencil=" << depth.stencilEnable << " bounds=" << depth.boundsEnable << " separate_back=" << depth.separateBackFace << '\n';
    for (std::size_t index = 0; index < result.targetCount; ++index) {
        const auto& target = result.targets[index];
        output << "target=" << index << " format=" << result.formats[index] << " blend=" << target.enable << " source=" << target.source << " destination=" << target.destination << " operation=" << target.operation << " source_alpha=" << target.sourceAlpha << " destination_alpha=" << target.destinationAlpha << " alpha_operation=" << target.alphaOperation << " logic=" << target.logicEnable << ':' << target.logicOperation << " write_mask=" << static_cast<std::uint32_t>(target.writeMask) << '\n';
    }
    return output.str();
}
