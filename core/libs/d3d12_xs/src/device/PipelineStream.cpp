#include "Device.hpp"
#include "../pipeline/State.hpp"
#include "../pipeline/graphics/Creation.hpp"
#include "../pipeline/stream/Description.hpp"

HRESULT AnyBox::Graphics::Device::CreatePipelineStateX(Device* self, const D3D12_PIPELINE_STATE_STREAM_DESC* description, const Guid& interfaceId, void** pipeline) {
    if (!pipeline) throw std::invalid_argument("Pipeline stream output is null");
    *pipeline = nullptr;
    if (!self || !description || !description->pPipelineStateSubobjectStream) throw std::invalid_argument("Pipeline stream creation arguments are null");
    if (!PipelineState::Supports(interfaceId)) throw std::invalid_argument("Pipeline stream interface is unsupported");
    PipelineStream stream({static_cast<const std::byte*>(description->pPipelineStateSubobjectStream), description->SizeInBytes});
    auto kind = stream.GetKind();
    if (kind == PipelineStreamKind::Mesh) throw std::runtime_error("Xbox mesh pipeline creation is not implemented");
    if (kind == PipelineStreamKind::Graphics) {
        *pipeline = CreatePreparedGraphics(*self, stream);
        return S_OK;
    }
    auto compute = stream.GetComputeDescription();
    return CreateComputePipelineStateX(self, &compute, 0, nullptr, interfaceId, pipeline);
}
