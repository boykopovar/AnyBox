#include "Device.hpp"
#include "../pipeline/State.hpp"
#include "../pipeline/Offline.hpp"
#include <memory>

namespace {
struct RootDeleter {
    void operator()(AnyBox::Graphics::RootSignature* value) const { AnyBox::Graphics::RootSignature::Release(value); }
};
struct PipelineDeleter {
    void operator()(ID3D12PipelineState* value) const noexcept { value->Release(); }
};
}

HRESULT AnyBox::Graphics::Device::CreateComputePipelineStateX(Device* self, const D3D12_COMPUTE_PIPELINE_STATE_DESC* description, std::uint32_t extensionCount, const void* extensions, const Guid& interfaceId, void** pipeline) {
    if (!pipeline) throw std::invalid_argument("Compute pipeline output is null");
    *pipeline = nullptr;
    if (!description) throw std::invalid_argument("Compute pipeline description is null");
    if (!PipelineState::Supports(interfaceId)) throw std::invalid_argument("Compute pipeline creation interface is unsupported");
    if (description->NodeMask != 0 && description->NodeMask != 1) throw std::invalid_argument("Compute pipeline node mask is unsupported");
    if (extensionCount != 0) {
        if (!extensions) throw std::invalid_argument("Compute pipeline extensions are null");
        throw std::runtime_error("Xbox compute pipeline extensions are not implemented");
    }
    if (description->CachedPSO.CachedBlobSizeInBytes != 0) throw std::runtime_error("Xbox compute pipeline cache loading is not implemented");
    constexpr std::uint32_t toolDebugFlag = 1;
    if ((static_cast<std::uint32_t>(description->Flags) & ~toolDebugFlag) != 0) throw std::invalid_argument("Compute pipeline flags are unsupported");
    if (!description->CS.pShaderBytecode || description->CS.BytecodeLength == 0) throw std::invalid_argument("Compute shader bytecode is empty");
    auto code = std::span(static_cast<const std::byte*>(description->CS.pShaderBytecode), description->CS.BytecodeLength);
    auto prepared = RequiresPreparedCompute(*self, code);
    std::unique_ptr<RootSignature, RootDeleter> embedded;
    RootSignature* root;
    if (description->pRootSignature) root = RootSignature::From(description->pRootSignature, self);
    else {
        void* output = nullptr;
        CreateRootSignature(self, description->NodeMask, code.data(), code.size(), RootSignatureId, &output);
        embedded.reset(static_cast<RootSignature*>(output));
        root = embedded.get();
    }

    if (prepared) {
        auto offline = std::make_unique<OfflinePipeline>(*self, *root, LoadPreparedCompute(code));
        *pipeline = new PipelineState(std::move(offline), self, root);
        return S_OK;
    }
    auto native = *description;
    native.pRootSignature = root->GetHost();
    void* host = nullptr;
    auto result = self->host->CreateComputePipelineState(&native, NativeGuid(PipelineStateId), &host);
    std::unique_ptr<ID3D12PipelineState, PipelineDeleter> owned(static_cast<ID3D12PipelineState*>(host));
    CheckGraphicsResult(result, "ID3D12Device::CreateComputePipelineState");
    if (!host) throw std::runtime_error("Compute pipeline creation returned no interface");
    auto wrapper = std::make_unique<PipelineState>(owned.get(), self, root);
    owned.release();
    *pipeline = wrapper.release();
    return result;
}
