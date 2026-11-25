#include "Binding.hpp"
#include <stdexcept>

void AnyBox::Shader::ValidateNativeComputeRootLayout(const Graphics::Container::RootSignatureLayout& layout) {
    if ((layout.version != 1 && layout.version != 2) || (layout.flags & ~0x83000FFFu) != 0 || (layout.flags & 0x80) != 0) throw std::invalid_argument("Native compute root signature version or flags are invalid");
}

void AnyBox::Shader::ValidateNativeRootBinding(const Graphics::Container::RootSignatureLayout& layout, const Graphics::Container::NativeRootBinding& binding, std::optional<Graphics::Container::RootRangeKind> rangeKind) {
    using namespace Graphics::Container;
    if (binding.registerCount == 0) {
        if (binding.kind != NativeRootKind::ImmediateSampler) throw std::invalid_argument("Native root binding register range is empty");
        return;
    }
    if (binding.firstUserRegister >= 128 || binding.registerCount > 128u - binding.firstUserRegister) throw std::invalid_argument("Native root binding register range exceeds the artifact ABI capacity");
    if (binding.rootParameter >= layout.parameters.size()) throw std::invalid_argument("Native root binding parameter is outside the signature");
    const auto& parameter = layout.parameters[binding.rootParameter];
    if (parameter.visibility != 0) throw std::invalid_argument("Native compute root binding is hidden by its shader visibility");
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
            if (rangeKind && range.kind != *rangeKind) continue;
            if (binding.shaderRegister < range.firstRegister || binding.shaderRegister >= limit) continue;
            auto stride = range.kind == RootRangeKind::Sampler ? 4u : 8u;
            found |= (offset + binding.shaderRegister - range.firstRegister) * stride == binding.tableOffset;
        }
        if (!found) throw std::invalid_argument("Native descriptor table binding has no matching register and offset");
        return;
    }
    if (binding.tableOffset != 0 || binding.shaderRegister != parameter.shaderRegister) throw std::invalid_argument("Native direct root binding register or offset disagrees with its signature");
    if (binding.kind == NativeRootKind::Constants) {
        if (parameter.kind != RootParameterKind::Constants || parameter.wordCount == 0 || binding.registerCount != parameter.wordCount) throw std::invalid_argument("Native root constants do not match their declared word count");
    } else if (binding.kind == NativeRootKind::Descriptor) {
        if (binding.registerCount != 2 || (parameter.kind != RootParameterKind::ConstantBuffer && parameter.kind != RootParameterKind::ShaderResource && parameter.kind != RootParameterKind::UnorderedAccess)) throw std::invalid_argument("Native direct descriptor binding disagrees with its root parameter");
    } else throw std::runtime_error("Native immediate, resource heap and sampler heap root bindings are not implemented");
}
