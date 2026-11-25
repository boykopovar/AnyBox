#include <anybox/shader/NativeRootArgumentWriter.hpp>
#include "Binding.hpp"
#include <array>
#include <optional>
#include <stdexcept>

AnyBox::Shader::NativeRootArgumentWriter::NativeRootArgumentWriter(const Graphics::Container::RootSignatureLayout& layout, std::span<const Graphics::Container::NativeRootBinding> roots, std::span<const std::uint32_t> registers, NativeRootBindingMode mode, std::uint32_t visibility) {
    using namespace Graphics::Container;
    if (mode != NativeRootBindingMode::LogicalRegisters && mode != NativeRootBindingMode::SerializedSlots) throw std::invalid_argument("Native root writer binding mode is invalid");
    ValidateNativeRootLayout(layout, visibility);
    if (registers.size() > 128) throw std::length_error("Native root register selection exceeds the artifact ABI capacity");
    for (const auto& binding : roots) ValidateNativeRootBinding(layout, binding, std::nullopt, mode, visibility);
    std::array<bool, 128> selected{};
    copies.reserve(registers.size());
    for (auto index : registers) {
        if (index >= selected.size() || selected[index]) throw std::invalid_argument("Native root register selection is invalid or repeated");
        selected[index] = true;
        std::optional<Copy> copy;
        for (const auto& binding : roots) {
            if (index < binding.firstUserRegister || index - binding.firstUserRegister >= binding.registerCount) continue;
            const auto& parameter = layout.parameters[binding.rootParameter];
            auto count = parameter.kind == RootParameterKind::Constants ? parameter.wordCount : parameter.kind == RootParameterKind::Table ? 1u : 2u;
            Copy candidate{parameter.kind, binding.rootParameter, index - binding.firstUserRegister, count};
            if (copy && (*copy != candidate || binding.kind != NativeRootKind::Table)) throw std::invalid_argument("Native root register has conflicting argument mappings");
            copy = candidate;
        }
        if (!copy) throw std::runtime_error("Native root register has no argument mapping");
        copies.push_back(*copy);
    }
}

void AnyBox::Shader::NativeRootArgumentWriter::Write(const Graphics::Commands::RootState& state, std::span<std::uint32_t> destination) const {
    if (destination.size() != copies.size()) throw std::invalid_argument("Native root writer destination size is invalid");
    for (std::size_t index = 0; index < copies.size(); ++index) {
        const auto& copy = copies[index];
        if (state.GetKind(copy.parameter) != copy.kind || state.GetWordCount(copy.parameter) != copy.wordCount) throw std::invalid_argument("Native root writer state differs from its pipeline signature");
        auto value = state.Read(copy.parameter, copy.word, 1).front();
        if (copy.kind == Graphics::Container::RootParameterKind::Table && (value == 0 || value % 16 != 0)) throw std::invalid_argument("Native root descriptor table address is null or misaligned");
        destination[index] = value;
    }
}
