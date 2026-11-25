#pragma once

#include "Prepared.hpp"
#include "../../root/Signature.hpp"
#include <anybox/shader/NativeComputeArguments.hpp>
#include <anybox/shader/runtime/ComputePipeline.hpp>

namespace AnyBox::Graphics {
class NativeComputePipeline {
public:
    NativeComputePipeline(Device& device, const RootSignature& root, PreparedNativeCompute prepared, std::span<const Container::NativeRootBinding> roots) : resourcePlan(std::move(prepared.resources)), arguments(resourcePlan, root.GetLayout(), roots, prepared.shader.userRegisters), pipeline(device.GetGpuContext(), prepared.shader) {}
    const Shader::Runtime::ComputePipeline& GetPipeline() const noexcept { return pipeline; }
    const Shader::NativeComputeArguments& GetArguments() const noexcept { return arguments; }

private:
    ShaderRecompiler::IrResourcePlan resourcePlan;
    Shader::NativeComputeArguments arguments;
    Shader::Runtime::ComputePipeline pipeline;
};
}
