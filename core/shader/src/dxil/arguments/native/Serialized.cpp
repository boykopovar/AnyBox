#include <anybox/shader/SerializedRootArguments.hpp>
#include "Binding.hpp"
#include <stdexcept>

namespace {
using namespace AnyBox::Graphics::Container;

RootRangeKind RangeKind(NativeResourceKind kind) {
    switch (kind) {
    case NativeResourceKind::ShaderResource: return RootRangeKind::ShaderResource;
    case NativeResourceKind::UnorderedAccess: return RootRangeKind::UnorderedAccess;
    case NativeResourceKind::ConstantBuffer: return RootRangeKind::ConstantBuffer;
    case NativeResourceKind::Sampler: return RootRangeKind::Sampler;
    default: throw std::invalid_argument("Serialized shader resource class is invalid");
    }
}
}

std::vector<AnyBox::Graphics::Container::NativeRootBinding> AnyBox::Shader::BuildSerializedComputeRoots(std::span<const Graphics::Container::NativeResourceBinding> resources, const Graphics::Container::RootSignatureLayout& layout) {
    return BuildSerializedStageRoots(resources, layout, 0);
}

std::vector<AnyBox::Graphics::Container::NativeRootBinding> AnyBox::Shader::BuildSerializedStageRoots(std::span<const Graphics::Container::NativeResourceBinding> resources, const Graphics::Container::RootSignatureLayout& layout, std::uint32_t visibility) {
    using namespace Graphics::Container;
    ValidateNativeRootLayout(layout, visibility);
    std::vector<NativeRootBinding> output;
    output.reserve(resources.size());
    for (const auto& resource : resources) {
        auto kind = RangeKind(resource.kind);
        NativeRootBinding binding{resource.rootKind, resource.tableOffset, 0, resource.firstShaderRegister, resource.firstUserRegister, resource.rootParameter};
        if (resource.rootKind == NativeRootKind::ImmediateSampler) {
            if (resource.kind != NativeResourceKind::Sampler || resource.rootParameter >= layout.staticSamplerCount || resource.descriptorWords != 4) throw std::invalid_argument("Serialized immediate sampler binding is invalid");
        } else {
            if (resource.rootParameter >= layout.parameters.size()) throw std::invalid_argument("Serialized shader resource parameter is outside its root signature");
            const auto& parameter = layout.parameters[resource.rootParameter];
            if (resource.rootKind == NativeRootKind::Table) {
                auto words = resource.kind == NativeResourceKind::ShaderResource || resource.kind == NativeResourceKind::UnorderedAccess ? resource.descriptorWords == 4 || resource.descriptorWords == 8 : resource.descriptorWords == 4;
                if (!words) throw std::runtime_error("Serialized shader table descriptor width is not implemented");
                binding.registerCount = 1;
            } else if (resource.rootKind == NativeRootKind::Constants) {
                if (resource.kind != NativeResourceKind::ConstantBuffer) throw std::invalid_argument("Serialized root constants are not a constant buffer");
                binding.registerCount = parameter.wordCount;
            } else if (resource.rootKind == NativeRootKind::Descriptor) {
                auto descriptor = parameter.kind == RootParameterKind::ShaderResource || parameter.kind == RootParameterKind::UnorderedAccess || parameter.kind == RootParameterKind::ConstantBuffer;
                if (resource.kind == NativeResourceKind::Sampler || !descriptor || resource.descriptorWords != 4) throw std::invalid_argument("Serialized direct descriptor root parameter or width is invalid");
                binding.registerCount = 2;
            } else throw std::runtime_error("Serialized shader resource and sampler heaps are not implemented");
        }
        ValidateNativeRootBinding(layout, binding, kind, NativeRootBindingMode::SerializedSlots, visibility);
        output.push_back(binding);
    }
    return output;
}
