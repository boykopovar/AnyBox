#include "Graphics.hpp"
#include "../Files.hpp"
#include "../AuditTarget.hpp"
#include <anybox/shader/SerializedGraphicsCompiler.hpp>
#include <anybox/shader/NativeShaderArguments.hpp>
#include <iostream>

void AnyBox::Shader::Tools::AuditSerializedPixel(std::span<const std::string_view> arguments) {
    if (arguments.size() != 6) throw std::invalid_argument("Usage: --serialized-pixel-audit <state> <auxiliary> <code> <metadata> <root>");
    auto state = ReadInput(arguments[1]);
    auto auxiliary = ReadInput(arguments[2]);
    auto code = ReadInput(arguments[3]);
    auto metadata = ReadInput(arguments[4]);
    auto root = Graphics::Container::ReadRootSignatureLayout(ReadInput(arguments[5]));
    auto compiled = CompileSerializedPixel({state, code, metadata}, auxiliary, root, AuditTarget());
    NativeShaderArguments argumentsPlan(compiled.resources, root, compiled.roots, compiled.shader.bindings.layout.userDataRegisters, NativeRootBindingMode::SerializedSlots);
    std::cout << "words=" << compiled.shader.code.size() << " buffers=" << compiled.resources.info.buffers.size() << " images=" << compiled.resources.info.images.size() << " samplers=" << compiled.resources.info.samplers.size() << " arguments=" << compiled.shader.bindings.layout.userDataRegisters.size() << " system_registers=" << argumentsPlan.GetSystemRegisters().size() << '\n';
    if (!std::cout) throw std::runtime_error("Cannot write serialized pixel compilation results");
}
