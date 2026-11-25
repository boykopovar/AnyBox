#include <anybox/shader/NativeArguments.hpp>
#include <anybox/shader/NativeRootArgumentWriter.hpp>
#include "../Buffers.hpp"
#include <array>

void AnyBox::Shader::BindNativeRawRootArguments(NativeComputeTranslation& translation, std::span<const Graphics::Container::NativeRootBinding> roots, const Graphics::Container::RootSignatureLayout& layout, NativeRootBindingMode mode) {
    using namespace ShaderRecompiler;
    if (translation.rootArgumentsBound) throw std::logic_error("Native root arguments were already bound");
    auto& program = translation.program;
    if (program.Resources().srtPlanComplete || program.Resources().resourceTrackingComplete) throw std::logic_error("Native raw root arguments require an unplanned program");
    std::array<bool, 128> selected{};
    for (const auto& block : program.Blocks()) {
        for (const auto* value : block->Instructions()) {
            if (value->Opcode() != IrOpcode::GetUserData) continue;
            auto index = value->Argument(0)->Register().index;
            if (index >= selected.size()) throw std::invalid_argument("Native raw root argument register exceeds the artifact ABI capacity");
            selected[index] = true;
        }
    }
    std::vector<std::uint32_t> registers;
    for (std::uint32_t index = 0; index < selected.size(); ++index) if (selected[index]) registers.push_back(index);
    (void)NativeRootArgumentWriter(layout, roots, registers, mode);
    PrepareNativeAddressLoads(program);
    MarkNativeBufferDescriptors(program);
    translation.rootArgumentsBound = true;
}
