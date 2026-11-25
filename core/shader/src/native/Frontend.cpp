#include <anybox/shader/Native.hpp>
#include <anybox/graphics/container/Native.hpp>
#include "Decode.hpp"
#include <RdnaDecoder/RdnaInstructionDecoder.hpp>
#include <ControlFlow/GraphBuilder.hpp>
#include <ControlFlow/Structurizer.hpp>
#include <stdexcept>

AnyBox::Shader::NativeFrontend::NativeFrontend(std::span<const std::byte> program) {
    Graphics::Container::FssmView view(program);
    initialize(view.GetCode(), view.GetWaveSize());
}

AnyBox::Shader::NativeFrontend::NativeFrontend(std::span<const std::byte> instructions, std::uint32_t selectedWaveSize) {
    initialize(instructions, selectedWaveSize);
}

void AnyBox::Shader::NativeFrontend::initialize(std::span<const std::byte> bytes, std::uint32_t selectedWaveSize) {
    if (ShaderRecompiler::GcnGuestShaders()) throw std::logic_error("Xbox native program requires RDNA decoding mode");
    if (selectedWaveSize != 32 && selectedWaveSize != 64) throw std::invalid_argument("Xbox native program wave size is invalid");
    if (bytes.empty() || bytes.size() % 4 != 0) throw std::invalid_argument("Xbox native instruction stream size is invalid");
    waveSize = selectedWaveSize;
    code.resize(bytes.size() / 4);
    for (std::size_t word = 0; word < code.size(); ++word) {
        for (unsigned byte = 0; byte < 4; ++byte) code[word] |= std::to_integer<std::uint32_t>(bytes[word * 4 + byte]) << (byte * 8);
    }
    decoded = DecodeNativeProgram(code);
    graph = ShaderRecompiler::GraphBuilder{}.Build(decoded);
    if (graph.unsupported) throw std::runtime_error(graph.unsupportedReason);
    ShaderRecompiler::Structurizer{}.Structurize(graph);
    if (graph.unsupported) throw std::runtime_error(graph.unsupportedReason);
}
