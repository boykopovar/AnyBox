#include "Map.hpp"
#include "../Optimize.hpp"
#include "../Buffers.hpp"
#include <Recompiler.hpp>
#include <IntermediateRepresentation/IrBuilder.hpp>
#include <algorithm>
#include <unordered_set>

namespace {
using namespace ShaderRecompiler;

bool Immediate(const IrValue* value, std::uint32_t expected) {
    value = value->Resolve();
    return value->Type() == IrType::U32 && value->HasImmediate() && value->ImmediateU32() == expected;
}

bool UserData(const IrValue* value, unsigned index) {
    value = value->Resolve();
    return value->Opcode() == IrOpcode::GetUserData && value->ArgumentCount() == 1 && value->Argument(0)->Register().index == index;
}

bool SpillAddress(const IrValue* value) {
    value = value->Resolve();
    if (value->Opcode() != IrOpcode::GetAddressResource || value->ArgumentCount() != 2 || !UserData(value->Argument(0), 0)) return false;
    auto* high = value->Argument(1)->Resolve();
    if (UserData(high, 1)) return true;
    if (high->Opcode() != IrOpcode::BitwiseAnd32 || high->ArgumentCount() != 2) return false;
    return (UserData(high->Argument(0), 1) && Immediate(high->Argument(1), 255)) || (UserData(high->Argument(1), 1) && Immediate(high->Argument(0), 255));
}

void BindArguments(IrProgram& program, std::span<const AnyBox::Graphics::Container::NativeRootBinding> roots, const AnyBox::Shader::RootArgumentPlan& plan, std::uint32_t initialRegisters, bool scalarBuffers) {
    using namespace AnyBox::Shader;
    auto mapping = MapNativeArguments(roots, plan);
    bool spillPointer = std::ranges::none_of(roots, [](const auto& root) { return root.firstUserRegister < 2; });
    for (const auto& root : roots) {
        if (root.firstUserRegister < 16 && std::min<std::uint32_t>(16, root.firstUserRegister + root.registerCount) > initialRegisters) throw std::invalid_argument("Native root arguments overlap initial system registers");
    }
    if (program.Resources().srtPlanComplete || program.Resources().resourceTrackingComplete) throw std::logic_error("Native root arguments require an unplanned program");
    std::vector<IrValue*> original;
    std::unordered_set<std::uint32_t> originalIds;
    std::vector<IrValue*> reads;
    for (const auto& block : program.Blocks()) {
        for (auto* value : block->Instructions()) {
            if (value->Opcode() == IrOpcode::GetUserData) {
                original.push_back(value);
                originalIds.insert(value->Id());
            }
            if (spillPointer && value->Opcode() == IrOpcode::LoadAddressU32 && SpillAddress(value->Argument(0))) reads.push_back(value);
        }
    }
    IrBuilder builder(program);
    builder.SetInsertionPoint(*program.BlockOrder().front());
    for (auto* read : reads) {
        auto flags = read->Flags<MemoryFlags>();
        const auto& memory = program.Resources().memoryInfo.at(flags.index);
        auto* enabled = read->Argument(3)->Resolve();
        if (memory.kind != ResourceKind::ScalarAddress || memory.offset % 4 != 0 || !Immediate(read->Argument(1), 0) || !Immediate(read->Argument(2), 0) || !enabled->HasImmediate() || !enabled->ImmediateBool()) throw std::runtime_error("Native dynamic or conditional root argument loads are not implemented");
        auto index = static_cast<std::uint64_t>(memory.offset) / 4 + 16;
        if (index >= mapping.size() || mapping[index] == UINT32_MAX) throw std::runtime_error("Native root spill load has no logical argument mapping");
        auto& replacement = builder.GetUserData(static_cast<ScalarReg>(mapping[index]));
        read->ReplaceAllUsesWith(&replacement);
        read->Parent()->RemoveInstruction(read);
        read->Invalidate();
    }
    for (auto* value : original) {
        auto index = value->Argument(0)->Register().index;
        if (index >= 16 || mapping[index] == UINT32_MAX) continue;
        auto& replacement = builder.GetUserData(static_cast<ScalarReg>(mapping[index]));
        value->ReplaceAllUsesWith(&replacement);
    }
    program.Resources().userDataCount = plan.GetUserRegisterCount();
    OptimizeNativeProgram(program, program.WaveSize());
    for (const auto& block : program.Blocks()) {
        for (auto* value : block->Instructions()) {
            if (originalIds.contains(value->Id())) throw std::runtime_error("Native initial user data remains outside the logical root argument mapping");
            if (BufferAccessOf(value->Opcode()) != BufferAccess::None) {
                auto& memory = program.Resources().memoryInfo.at(value->Flags<MemoryFlags>().index);
                if (!scalarBuffers && memory.kind == ResourceKind::ScalarBuffer) throw std::runtime_error("Native scalar buffer root argument accesses are not implemented");
            }
        }
    }
    PrepareNativeAddressLoads(program);
    MarkNativeBufferDescriptors(program);
}
}

void AnyBox::Shader::BindNativeRootArguments(NativeComputeTranslation& translation, std::span<const Graphics::Container::NativeRootBinding> roots, const RootArgumentPlan& plan) {
    if (translation.rootArgumentsBound) throw std::logic_error("Native root arguments were already bound");
    BindArguments(translation.program, roots, plan, static_cast<std::uint32_t>(translation.input.workgroupRegister), false);
    translation.rootArgumentsBound = true;
}

void AnyBox::Shader::BindNativeGraphicsRootArguments(ShaderRecompiler::IrProgram& program, std::span<const Graphics::Container::NativeRootBinding> roots, const RootArgumentPlan& plan) {
    auto stage = program.Resources().stage;
    if (stage != ShaderRecompiler::IrShaderStage::Vertex && stage != ShaderRecompiler::IrShaderStage::Pixel) throw std::invalid_argument("Native graphics root arguments require a vertex or pixel program");
    BindArguments(program, roots, plan, program.Resources().userDataCount, true);
}
