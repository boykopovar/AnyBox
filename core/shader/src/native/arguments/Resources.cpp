#include <anybox/shader/NativeArguments.hpp>
#include <algorithm>
#include <stdexcept>

std::vector<AnyBox::Shader::BufferBinding> AnyBox::Shader::ReadNativeDirectBufferBindings(const Graphics::Container::NativeBindings& bindings, const Graphics::Container::RootSignatureLayout& root, std::uint32_t visibility) {
    using namespace Graphics::Container;
    if (visibility != 1 && visibility != 5) throw std::invalid_argument("Native graphics resource visibility is invalid");
    std::vector<BufferBinding> output;
    for (const auto& resource : bindings.resources) {
        if (resource.rootParameter >= root.parameters.size()) throw std::invalid_argument("Native graphics resource root parameter is outside the signature");
        const auto& parameter = root.parameters[resource.rootParameter];
        if (parameter.visibility != 0 && parameter.visibility != visibility) throw std::invalid_argument("Native graphics resource root visibility disagrees with the shader stage");
        if (resource.rootKind != NativeRootKind::Descriptor) throw std::runtime_error("Native graphics resource tables, constants and heap resources are not implemented");
        if (resource.kind != NativeResourceKind::ShaderResource && resource.kind != NativeResourceKind::UnorderedAccess) throw std::runtime_error("Native graphics resource is not a direct shader buffer");
        auto kind = resource.kind == NativeResourceKind::ShaderResource ? RootParameterKind::ShaderResource : RootParameterKind::UnorderedAccess;
        if (parameter.kind != kind || parameter.shaderRegister != resource.firstShaderRegister || resource.tableOffset != 0 || resource.stride != 1 || resource.descriptorWords != 4) throw std::invalid_argument("Native graphics direct buffer metadata disagrees with its root signature");
        auto match = std::ranges::find_if(bindings.roots, [&](const auto& binding) { return binding.rootParameter == resource.rootParameter && binding.shaderRegister == resource.firstShaderRegister && binding.kind == resource.rootKind && binding.firstUserRegister == resource.firstUserRegister; });
        if (match == bindings.roots.end() || match->registerCount != 2 || match->tableOffset != 0) throw std::invalid_argument("Native graphics direct buffer has no matching root argument");
        BufferBinding buffer{resource.kind == NativeResourceKind::ShaderResource ? ResourceClass::ShaderResource : ResourceClass::UnorderedAccess, static_cast<std::uint32_t>(output.size()), parameter.space, parameter.shaderRegister, 1, BufferKind::Raw};
        if (std::ranges::any_of(output, [&](const auto& previous) { return previous.resourceClass == buffer.resourceClass && previous.firstRegister == buffer.firstRegister && previous.space == buffer.space; })) throw std::invalid_argument("Native graphics direct buffer metadata is duplicated");
        output.push_back(buffer);
    }
    return output;
}
