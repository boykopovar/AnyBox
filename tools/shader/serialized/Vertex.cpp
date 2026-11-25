#include "Graphics.hpp"
#include "../Files.hpp"
#include "../AuditTarget.hpp"
#include <anybox/shader/SerializedGraphicsCompiler.hpp>
#include <anybox/shader/NativeShaderArguments.hpp>
#include <charconv>
#include <iostream>

namespace {
std::int32_t Integer(std::string_view text) {
    std::int32_t value;
    auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) throw std::invalid_argument("Serialized graphics argument is not an integer");
    return value;
}
}

bool AnyBox::Shader::Tools::RunSerializedGraphicsCommand(std::span<const std::string_view> arguments) {
    if (!arguments.empty() && arguments[0] == "--serialized-pixel-audit") {
        AuditSerializedPixel(arguments);
        return true;
    }
    if (arguments.empty() || arguments[0] != "--serialized-vertex-audit") return false;
    if (arguments.size() != 8) throw std::invalid_argument("Usage: --serialized-vertex-audit <state> <code> <metadata> <root> <wave> <vertex-offset-sgpr> <instance-offset-sgpr>");
    auto state = ReadInput(arguments[1]);
    auto code = ReadInput(arguments[2]);
    auto metadata = ReadInput(arguments[3]);
    auto root = Graphics::Container::ReadRootSignatureLayout(ReadInput(arguments[4]));
    auto wave = Integer(arguments[5]);
    if (wave != 32 && wave != 64) throw std::invalid_argument("Serialized vertex wave size must be 32 or 64");
    NativeVertexSystemRegisters system{Integer(arguments[6]), Integer(arguments[7])};
    auto target = AuditTarget();
    std::vector<std::uint32_t> capabilities(target.supportedCapabilities.begin(), target.supportedCapabilities.end());
    capabilities.push_back(spv::CapabilityClipDistance);
    capabilities.push_back(spv::CapabilityCullDistance);
    target.supportedCapabilities = capabilities;
    ShaderRecompiler::ShaderVertexInputInfo input;
    input.hostSubgroupSize = target.subgroupSize;
    auto compiled = CompileSerializedVertex({state, code, metadata, static_cast<std::uint32_t>(wave)}, root, input, target, system);
    NativeShaderArguments argumentsPlan(compiled.resources, root, compiled.roots, compiled.shader.bindings.layout.userDataRegisters, NativeRootBindingMode::SerializedSlots);
    std::cout << "words=" << compiled.shader.code.size() << " buffers=" << compiled.resources.info.buffers.size() << " images=" << compiled.resources.info.images.size() << " samplers=" << compiled.resources.info.samplers.size() << " arguments=" << compiled.shader.bindings.layout.userDataRegisters.size() << " system_registers=" << argumentsPlan.GetSystemRegisters().size() << '\n';
    if (!std::cout) throw std::runtime_error("Cannot write serialized vertex compilation results");
    return true;
}
