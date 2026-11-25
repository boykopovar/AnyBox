#include "../Serialized.hpp"
#include <anybox/graphics/container/SerializedStage.hpp>
#include <anybox/shader/Native.hpp>
#include <anybox/shader/SerializedRootArguments.hpp>
#include <anybox/shader/NativeRootArgumentWriter.hpp>
#include <RdnaDecoder/RdnaInstructionDecoder.hpp>
#include <array>
#include <sstream>

std::string AnyBox::Shader::Tools::InspectSerializedStageInstructions(std::span<const std::byte> code, std::span<const std::byte> records, std::uint32_t waveSize) {
    auto metadata = Graphics::Container::ReadSerializedStageMetadata(code, records);
    NativeFrontend frontend(code.first(static_cast<std::size_t>(metadata.codeWords) * 4), waveSize);
    std::ostringstream output;
    output << "wave=" << waveSize << " instructions=" << frontend.GetDecoded().instructions.size() << " blocks=" << frontend.GetGraph().blocks.size() << '\n';
    for (const auto& instruction : frontend.GetDecoded().instructions) output << ShaderRecompiler::RdnaInstructionToString(instruction) << '\n';
    return output.str();
}

std::string AnyBox::Shader::Tools::InspectSerializedStageRoots(std::span<const std::byte> code, std::span<const std::byte> records, std::span<const std::byte> root, std::uint32_t visibility) {
    auto metadata = Graphics::Container::ReadSerializedStageMetadata(code, records);
    auto layout = Graphics::Container::ReadRootSignatureLayout(root);
    auto roots = BuildSerializedStageRoots(metadata.resources, layout, visibility);
    std::array<bool, 128> selected{};
    for (const auto& binding : roots) {
        for (std::uint32_t word = 0; word < binding.registerCount; ++word) selected.at(binding.firstUserRegister + word) = true;
    }
    std::vector<std::uint32_t> registers;
    for (std::uint32_t index = 0; index < selected.size(); ++index) if (selected[index]) registers.push_back(index);
    NativeRootArgumentWriter writer(layout, roots, registers, NativeRootBindingMode::SerializedSlots, visibility);
    std::ostringstream output;
    output << "visibility=" << visibility << " roots=" << roots.size() << " user_registers=" << writer.GetWordCount() << '\n';
    for (const auto& binding : roots) output << "kind=" << static_cast<std::uint32_t>(binding.kind) << " parameter=" << static_cast<std::uint32_t>(binding.rootParameter) << " sgpr=" << static_cast<std::uint32_t>(binding.firstUserRegister) << " words=" << binding.registerCount << " table_offset=" << binding.tableOffset << '\n';
    return output.str();
}
