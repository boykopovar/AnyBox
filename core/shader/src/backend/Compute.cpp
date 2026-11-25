#include <anybox/shader/ComputeCompiler.hpp>
#include "Validation.hpp"
#include "choices/Choice.hpp"
#include "reductions/Optimize.hpp"
#include <Optimization/ConstantFolder.hpp>
#include <Optimization/DeadCodeEliminator.hpp>
#include <Optimization/ResourceTracker.hpp>
#include <Optimization/ResourceMaterializer.hpp>
#include <Optimization/RedundantValueEliminator.hpp>
#include <Optimization/ShaderInfoCollector.hpp>
#include <Optimization/SrtWalker.hpp>
#include <spirv/unified1/spirv.hpp>
#include <algorithm>
#include <limits>

AnyBox::Shader::ComputeCompilation AnyBox::Shader::CompileCompute(ShaderRecompiler::IrProgram& program, const ShaderRecompiler::ShaderComputeInputInfo& input, const ShaderRecompiler::SpirvTargetOptions& target, const ShaderRecompiler::BindingLayout& layout) {
    using namespace ShaderRecompiler;
    if (program.Resources().stage != IrShaderStage::Unknown && program.Resources().stage != IrShaderStage::Compute) throw std::invalid_argument("Compute compilation received a different shader stage");
    if (input.waveSize != 32 && input.waveSize != 64) throw std::invalid_argument("Compute wave size is invalid");
    if (input.hostSubgroupSize != target.subgroupSize) throw std::invalid_argument("Compute input and target subgroup sizes do not match");
    if (std::ranges::any_of(input.threadsNum, [](auto size) { return size == 0; })) throw std::invalid_argument("Compute workgroup dimensions must be nonzero");
    if (input.ldsSizeDwords > std::numeric_limits<std::uint32_t>::max() / 4) throw std::invalid_argument("Compute shared memory size overflows its byte representation");
    if (program.Resources().srtPlanComplete || program.Resources().resourceTrackingComplete || program.Metadata().shaderInfoComplete || program.Metadata().bindingLayoutComplete) throw std::logic_error("Compute compilation requires an unplanned program");

    program.Resources().stage = IrShaderStage::Compute;
    program.SetWaveSize(input.waveSize);
    program.Info().sharedMemoryBytes = input.ldsSizeDwords * 4;
    if (program.Info().scratchDwords != 0 && input.scratchSizeDwords != 0 && program.Info().scratchDwords != input.scratchSizeDwords) throw std::invalid_argument("Local storage and compute scratch sizes do not match");
    if (program.Info().scratchDwords == 0) program.Info().scratchDwords = input.scratchSizeDwords;
    ValidateProgram(program, true);
    for (const auto& block : program.Blocks()) {
        for (auto iterator = block->Instructions().begin(); iterator != block->Instructions().end();) {
            auto* instruction = *iterator++;
            auto opcode = instruction->Opcode();
            if (opcode == IrOpcode::Branch || opcode == IrOpcode::BranchConditional || opcode == IrOpcode::Return) block->RemoveInstruction(instruction);
        }
    }
    ConstantFolder{}.Fold(program);
    ResolveControlFlowIdentities(program);
    DeadCodeEliminator{}.RemoveIdentities(program);
    DeadCodeEliminator{}.Eliminate(program);
    if (PlanDescriptorChoices(program) != 0) DeadCodeEliminator{}.Eliminate(program);
    ValidateProgram(program, true);
    SrtWalker{}.BuildPlan(program);
    ResourceTracker{}.Track(program);
    auto nativeOffsets = target.nonConstantImageOffsets && std::ranges::find(target.supportedCapabilities, static_cast<std::uint32_t>(spv::CapabilityImageGatherExtended)) != target.supportedCapabilities.end();
    ResourceMaterializer{}.ApplyStaticInterface(program, nativeOffsets);
    DeadCodeEliminator{}.RemoveIdentities(program);
    (void)RedundantValueEliminator{}.Eliminate(program);
    DeadCodeEliminator{}.Eliminate(program);
    ShaderStageInputInfo stage{nullptr, nullptr, &input};
    ShaderInfoCollector{}.Collect(program, stage);
    auto allocation = BindingAllocator{}.Allocate(program, layout, target.registrationModules);
    ComputeCompilation result;
    result.hostSubgroupSize = input.hostSubgroupSize;
    result.bindings = std::move(static_cast<CompiledBindingLayout&>(allocation));
    result.code = SpirvEmitter{}.Emit(program, stage, result.bindings, target, &result.info);
    ValidateSpirv(result.code, target.vulkanVersion);
    if (OptimizeWaveReductions(result.code, input, target)) ValidateSpirv(result.code, target.vulkanVersion);
    return result;
}
