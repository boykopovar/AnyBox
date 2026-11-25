#include "Serialized.hpp"
#include <anybox/graphics/container/Fspx.hpp>
#include <anybox/shader/Artifact.hpp>
#include <anybox/shader/Native.hpp>
#include <RdnaDecoder/RdnaInstructionDecoder.hpp>
#include <sstream>

namespace {
std::span<const std::byte> ReadPayload(std::span<const std::byte> data) {
    using namespace AnyBox::Graphics::Container;
    if (data.size() >= 4 && data[0] == std::byte{'D'} && data[1] == std::byte{'X'} && data[2] == std::byte{'B'} && data[3] == std::byte{'C'}) {
        DxbcView container(data);
        auto part = container.FindPart(0x43434341);
        if (!part) throw std::invalid_argument("Shader container has no serialized pipeline part");
        data = part->data;
    }
    return data;
}
}

std::string AnyBox::Shader::Tools::InspectSerializedCompute(std::span<const std::byte> data) {
    data = ReadPayload(data);
    Graphics::Container::FspxView view(data);
    std::ostringstream output;
    output << "dispatch_bytes=" << view.GetDispatchState().size() << " auxiliary_bytes=" << view.GetAuxiliaryState().size();
    output << " code_words=" << view.GetCode().size() / 4 << " record_bytes=" << view.GetRecords().size();
    output << " source=" << DigestName(HashShader(data)) << '\n';
    return output.str();
}

std::string AnyBox::Shader::Tools::InspectSerializedInstructions(std::span<const std::byte> data, std::uint32_t waveSize) {
    Graphics::Container::FspxView view(ReadPayload(data));
    NativeFrontend frontend(view.GetCode(), waveSize);
    std::ostringstream output;
    output << "wave=" << waveSize << " instructions=" << frontend.GetDecoded().instructions.size() << " blocks=" << frontend.GetGraph().blocks.size() << '\n';
    for (const auto& instruction : frontend.GetDecoded().instructions) output << ShaderRecompiler::RdnaInstructionToString(instruction) << '\n';
    return output.str();
}
