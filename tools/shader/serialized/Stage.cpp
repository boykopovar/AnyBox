#include "../Serialized.hpp"
#include <anybox/graphics/container/SerializedStage.hpp>
#include <anybox/shader/Native.hpp>
#include <RdnaDecoder/RdnaInstructionDecoder.hpp>
#include <sstream>

std::string AnyBox::Shader::Tools::InspectSerializedStageInstructions(std::span<const std::byte> code, std::span<const std::byte> records, std::uint32_t waveSize) {
    auto metadata = Graphics::Container::ReadSerializedStageMetadata(code, records);
    NativeFrontend frontend(code.first(static_cast<std::size_t>(metadata.codeWords) * 4), waveSize);
    std::ostringstream output;
    output << "wave=" << waveSize << " instructions=" << frontend.GetDecoded().instructions.size() << " blocks=" << frontend.GetGraph().blocks.size() << '\n';
    for (const auto& instruction : frontend.GetDecoded().instructions) output << ShaderRecompiler::RdnaInstructionToString(instruction) << '\n';
    return output.str();
}
