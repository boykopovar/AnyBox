#include "Binding.hpp"
#include <stdexcept>
#include <string>

void AnyBox::Shader::ValidateNativeRootLayout(const Graphics::Container::RootSignatureLayout& layout, std::uint32_t visibility) {
    if (visibility != 0 && visibility != 1 && visibility != 5) throw std::runtime_error("Native root shader stage is not implemented");
    if ((layout.version != 1 && layout.version != 2) || (layout.flags & ~0x83000FFFu) != 0 || (layout.flags & 0x80) != 0) throw std::invalid_argument("Native root signature version or flags are invalid");
    auto denied = visibility == 1 ? 2u : visibility == 5 ? 32u : 0u;
    if ((layout.flags & denied) != 0) throw std::invalid_argument("Native root signature denies access to the shader stage");
}

void AnyBox::Shader::ValidateNativeRootBinding(const Graphics::Container::RootSignatureLayout& layout, const Graphics::Container::NativeRootBinding& binding, std::optional<Graphics::Container::RootRangeKind> rangeKind, NativeRootBindingMode mode, std::uint32_t visibility) {
    using namespace Graphics::Container;
    if (mode != NativeRootBindingMode::LogicalRegisters && mode != NativeRootBindingMode::SerializedSlots) throw std::invalid_argument("Native root binding validation mode is invalid");
    if (binding.registerCount == 0) {
        if (binding.kind != NativeRootKind::ImmediateSampler) throw std::invalid_argument("Native root binding register range is empty");
        return;
    }
    if (binding.firstUserRegister >= 128 || binding.registerCount > 128u - binding.firstUserRegister) throw std::invalid_argument("Native root binding register range exceeds the artifact ABI capacity");
    if (binding.rootParameter >= layout.parameters.size()) throw std::invalid_argument("Native root binding parameter is outside the signature");
    const auto& parameter = layout.parameters[binding.rootParameter];
    if (parameter.visibility != 0 && parameter.visibility != visibility) throw std::invalid_argument("Native root binding is hidden by its shader visibility");
    if (binding.kind == NativeRootKind::Table) {
        if (parameter.kind != RootParameterKind::Table || binding.registerCount != 1) throw std::invalid_argument("Native descriptor table binding disagrees with its root parameter");
        std::uint64_t cursor = 0;
        bool found = false;
        for (const auto& range : parameter.ranges) {
            if (static_cast<unsigned>(range.kind) > 3 || range.count == 0 || (range.flags & ~0x1000Fu) != 0) throw std::invalid_argument("Native descriptor table range is invalid");
            std::uint64_t offset = range.tableOffset == UINT32_MAX ? cursor : range.tableOffset;
            auto limit = range.count == UINT32_MAX ? static_cast<std::uint64_t>(UINT32_MAX) + 1 : static_cast<std::uint64_t>(range.firstRegister) + range.count;
            cursor = range.count == UINT32_MAX ? static_cast<std::uint64_t>(UINT32_MAX) + 1 : offset + range.count;
            if (offset > UINT32_MAX || cursor > static_cast<std::uint64_t>(UINT32_MAX) + 1 || limit > static_cast<std::uint64_t>(UINT32_MAX) + 1) throw std::invalid_argument("Native descriptor table range overflows");
            if (mode == NativeRootBindingMode::SerializedSlots) { found = true; continue; }
            if (rangeKind && range.kind != *rangeKind) continue;
            auto stride = range.kind == RootRangeKind::Sampler ? 4u : 8u;
            if (binding.shaderRegister >= range.firstRegister && binding.shaderRegister < limit) found |= (offset + binding.shaderRegister - range.firstRegister) * stride == binding.tableOffset;
        }
        auto alignment = rangeKind && *rangeKind != RootRangeKind::Sampler ? 8u : 4u;
        if (mode == NativeRootBindingMode::SerializedSlots && binding.tableOffset % alignment != 0) throw std::invalid_argument("Serialized descriptor table slot is misaligned");
        if (!found) {
            auto detail = "Native descriptor table binding has no matching register and offset: parameter=" + std::to_string(binding.rootParameter) + " register=" + std::to_string(binding.shaderRegister) + " offset=" + std::to_string(binding.tableOffset) + " mode=" + std::to_string(static_cast<unsigned>(mode)) + " kind=" + (rangeKind ? std::to_string(static_cast<unsigned>(*rangeKind)) : "any");
            for (const auto& range : parameter.ranges) detail += " range(kind=" + std::to_string(static_cast<unsigned>(range.kind)) + ",register=" + std::to_string(range.firstRegister) + ",count=" + std::to_string(range.count) + ",offset=" + std::to_string(range.tableOffset) + ")";
            throw std::invalid_argument(detail);
        }
        return;
    }
    if (binding.tableOffset != 0 || (mode == NativeRootBindingMode::LogicalRegisters && binding.shaderRegister != parameter.shaderRegister)) throw std::invalid_argument("Native direct root binding register or offset disagrees with its signature");
    if (binding.kind == NativeRootKind::Constants) {
        if (parameter.kind != RootParameterKind::Constants || parameter.wordCount == 0 || binding.registerCount != parameter.wordCount) throw std::invalid_argument("Native root constants do not match their declared word count");
    } else if (binding.kind == NativeRootKind::Descriptor) {
        if (binding.registerCount != 2 || (parameter.kind != RootParameterKind::ConstantBuffer && parameter.kind != RootParameterKind::ShaderResource && parameter.kind != RootParameterKind::UnorderedAccess)) throw std::invalid_argument("Native direct descriptor binding disagrees with its root parameter");
    } else throw std::runtime_error("Native immediate, resource heap and sampler heap root bindings are not implemented");
}
