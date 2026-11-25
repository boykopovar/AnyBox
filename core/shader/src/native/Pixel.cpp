#include <anybox/shader/NativePixel.hpp>
#include "Options.hpp"
#include "Optimize.hpp"
#include "Slices.hpp"
#include "Buffers.hpp"
#include "PixelExports.hpp"
#include <Optimization/SsaBuilder.hpp>
#include <Optimization/HostInterpolationChecker.hpp>
#include <algorithm>
#include <spirv/unified1/spirv.hpp>

ShaderRecompiler::IrProgram AnyBox::Shader::LowerNativePixel(const NativeFrontend& frontend, const Graphics::Container::NativeStageState& state, const ShaderRecompiler::ShaderPixelInputInfo& input, std::uint32_t userRegisters, const ShaderRecompiler::SpirvTargetOptions& target) {
    using namespace ShaderRecompiler;
    if (state.stage != Graphics::Container::NativeHardwareStage::Pixel) throw std::invalid_argument("Native pixel translation received a different hardware stage");
    if (target.subgroupSize != 32 && target.subgroupSize != 64) throw std::invalid_argument("Native pixel target subgroup size is invalid");
    if (state.floatControl > 255 || input.inputNum > 32) throw std::invalid_argument("Native pixel float mode or interpolator count is invalid");
    if (target.float32FlushToZero && (target.float32DenormPreserve || (state.floatControl & 0x30) != 0)) throw std::invalid_argument("Native pixel target float controls disagree with the initial mode");
    ShaderFloatMode mode{state.floatControl, state.dx10Clamp, state.ieeeMode, false};
    auto options = MakeNativeOptions(ShaderStageKind::Pixel, frontend.GetWaveSize(), userRegisters, mode, target);
    options.inputInfo.pixel = &input;
    auto result = TranslateNativeProgram(frontend.GetDecoded(), frontend.GetGraph(), options);
    SsaBuilder{}.Rewrite(result);
    RemoveDisabledPixelExportSources(result);
    OptimizeNativeProgram(result, frontend.GetWaveSize());
    if (!HostInterpolationChecker{}.Lower(result, input, false, true)) {
        if (std::ranges::find(target.supportedCapabilities, static_cast<std::uint32_t>(spv::CapabilityFragmentBarycentricKHR)) == target.supportedCapabilities.end()) throw std::runtime_error("Native pixel interpolation requires fragment shader barycentric support");
        options.fragmentShaderBarycentricEnabled = true;
        result = TranslateNativeProgram(frontend.GetDecoded(), frontend.GetGraph(), options);
        SsaBuilder{}.Rewrite(result);
        RemoveDisabledPixelExportSources(result);
        OptimizeNativeProgram(result, frontend.GetWaveSize());
    }
    OptimizeNativeProgram(result, frontend.GetWaveSize());
    MarkNativeBufferDescriptors(result);
    return result;
}

AnyBox::Shader::StageCompilation AnyBox::Shader::CompileNativePixel(std::span<const std::byte> program, const ShaderRecompiler::ShaderPixelInputInfo& input, std::uint32_t userRegisters, const ShaderRecompiler::SpirvTargetOptions& target, const ShaderRecompiler::BindingLayout& layout) {
    auto state = Graphics::Container::ReadNativeStageState(program);
    NativeFrontend frontend(program);
    auto lowered = LowerNativePixel(frontend, state, input, userRegisters, target);
    return CompilePixel(lowered, input, target, layout);
}
