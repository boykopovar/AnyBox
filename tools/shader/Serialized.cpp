#include "Serialized.hpp"
#include "AuditTarget.hpp"
#include <anybox/graphics/container/Fspx.hpp>
#include <anybox/graphics/container/FspxDispatch.hpp>
#include <anybox/shader/Artifact.hpp>
#include <anybox/shader/Native.hpp>
#include <anybox/shader/NativeCompute.hpp>
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
    auto state = Graphics::Container::ReadSerializedComputeState(view.GetDispatchState());
    std::ostringstream output;
    output << "dispatch_bytes=" << view.GetDispatchState().size() << " auxiliary_bytes=" << view.GetAuxiliaryState().size();
    output << " code_words=" << view.GetCode().size() / 4 << " record_bytes=" << view.GetRecords().size();
    output << " wave=" << state.waveSize << " threads=" << state.shader.threads[0] << ',' << state.shader.threads[1] << ',' << state.shader.threads[2] << " shared_bytes=" << state.shader.declaredSharedBytes;
    output << " source=" << DigestName(HashShader(data)) << '\n';
    return output.str();
}

std::string AnyBox::Shader::Tools::InspectSerializedInstructions(std::span<const std::byte> data, std::uint32_t waveSize) {
    Graphics::Container::FspxView view(ReadPayload(data));
    if (Graphics::Container::ReadSerializedComputeState(view.GetDispatchState()).waveSize != waveSize) throw std::invalid_argument("Serialized shader wave size disagrees with its dispatch state");
    NativeFrontend frontend(view.GetCode(), waveSize);
    std::ostringstream output;
    output << "wave=" << waveSize << " instructions=" << frontend.GetDecoded().instructions.size() << " blocks=" << frontend.GetGraph().blocks.size() << '\n';
    for (const auto& instruction : frontend.GetDecoded().instructions) output << ShaderRecompiler::RdnaInstructionToString(instruction) << '\n';
    return output.str();
}

std::string AnyBox::Shader::Tools::AuditSerializedCompute(std::span<const std::byte> data) {
    Graphics::Container::FspxView view(ReadPayload(data));
    auto state = Graphics::Container::ReadSerializedComputeState(view.GetDispatchState());
    NativeFrontend frontend(view.GetCode(), state.waveSize);
    auto compiled = CompileNativeCompute(frontend, state.shader, AuditTarget(), {0, 0, 0, 128});
    const auto& layout = compiled.bindings.layout;
    std::ostringstream output;
    output << "words=" << compiled.code.size() << " images=" << layout.runtimeImageCount << " push_bytes=" << compiled.bindings.pushConstantSizeBytes << " user_registers=";
    for (auto index : layout.userDataRegisters) output << index << ',';
    output << "\ndescriptors=";
    for (const auto& descriptor : layout.descriptors) output << static_cast<std::uint32_t>(descriptor.kind) << ':' << descriptor.resources.size() << ',';
    output << '\n';
    return output.str();
}
