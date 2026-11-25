#include <anybox/shader/VertexCompiler.hpp>
#include "Bindings.hpp"
#include "GraphicsWave.hpp"
#include <stdexcept>

AnyBox::Shader::StageCompilation AnyBox::Shader::CompileVertex(ShaderRecompiler::IrProgram& program, const ShaderRecompiler::ShaderVertexInputInfo& input, const ShaderRecompiler::SpirvTargetOptions& target, const ShaderRecompiler::BindingLayout& layout) {
    using namespace ShaderRecompiler;
    if (program.Resources().stage != IrShaderStage::Vertex || input.logicalStage != IrShaderStage::Vertex) throw std::invalid_argument("Vertex compilation received a different shader stage");
    if (program.WaveSize() != 32 && program.WaveSize() != 64) throw std::invalid_argument("Vertex shader wave size is invalid");
    if (input.hostSubgroupSize != target.subgroupSize) throw std::invalid_argument("Vertex input and target subgroup sizes do not match");
    if (program.Info().scratchDwords != 0 && input.scratchSizeDwords != 0 && program.Info().scratchDwords != input.scratchSizeDwords) throw std::invalid_argument("Vertex shader scratch declarations disagree");
    if (program.Info().scratchDwords == 0) program.Info().scratchDwords = input.scratchSizeDwords;
    ShaderStageInputInfo stage{&input, nullptr, nullptr};
    StageCompilation result;
    result.bindings = PrepareShaderBindings(program, stage, target, layout);
    result.code = SpirvEmitter{}.Emit(program, stage, result.bindings, target, &result.info);
    ValidateGraphicsWave(result.code, program.WaveSize(), target);
    return result;
}
