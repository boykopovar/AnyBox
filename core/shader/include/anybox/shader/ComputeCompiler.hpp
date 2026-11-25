#pragma once

#include <SpirvBackend/SpirvEmitter.hpp>

namespace AnyBox::Shader {
struct ComputeCompilation {
    std::vector<std::uint32_t> code;
    ShaderRecompiler::CompiledBindingLayout bindings;
    ShaderRecompiler::SpirvEmitInfo info;
    std::uint32_t hostSubgroupSize = 0;
};

ComputeCompilation CompileCompute(ShaderRecompiler::IrProgram& program, const ShaderRecompiler::ShaderComputeInputInfo& input, const ShaderRecompiler::SpirvTargetOptions& target, const ShaderRecompiler::BindingLayout& layout);
}
