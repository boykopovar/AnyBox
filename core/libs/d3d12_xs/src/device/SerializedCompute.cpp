#include "Device.hpp"
#include "../pipeline/State.hpp"
#include "../pipeline/native/Description.hpp"
#include "../pipeline/native/Pipeline.hpp"
#include <anybox/graphics/container/FspxMetadata.hpp>
#include <anybox/shader/SerializedRootArguments.hpp>

HRESULT AnyBox::Graphics::Device::DeserializeComputePipelineStateX(Device* self, const SerializedComputeDescription* description, const Guid& interfaceId, void** pipeline) {
    if (!pipeline) throw std::invalid_argument("Serialized compute pipeline output is null");
    *pipeline = nullptr;
    if (!description) throw std::invalid_argument("Serialized compute pipeline description is null");
    if (!PipelineState::Supports(interfaceId)) throw std::invalid_argument("Serialized compute pipeline creation interface is unsupported");
    auto* root = RootSignature::From(description->root, self);
    auto source = ReadSerializedComputeSource(*description);
    auto metadata = Container::ReadSerializedComputeMetadata(source.code, source.metadata);
    auto roots = Shader::BuildSerializedComputeRoots(metadata.resources, root->GetLayout());
    auto subgroupSize = self->GetGpuContext().GetSubgroupProperties().subgroupSize;
    auto prepared = LoadPreparedNativeCompute(source, root->GetSourceDigest(), subgroupSize);
    auto native = std::make_unique<NativeComputePipeline>(*self, *root, std::move(prepared), roots);
    *pipeline = new PipelineState(std::move(native), self, root);
    return S_OK;
}
