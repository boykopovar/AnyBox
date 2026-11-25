#include "State.hpp"
#include "Offline.hpp"
#include "graphics/Offline.hpp"
#include "native/Pipeline.hpp"
#include <anybox/graphics/commands/Inline.hpp>

AnyBox::Graphics::PipelineState::PipelineState(ID3D12PipelineState* host, Device* device, RootSignature* root) : vtable(&GetPipelineTable()), host(host), device(device), root(root) {
    initialize();
}

AnyBox::Graphics::PipelineState::PipelineState(std::unique_ptr<OfflinePipeline> offline, Device* device, RootSignature* root) : vtable(&GetPipelineTable()), host(nullptr), device(device), root(root) {
    initialize();
    backend.offline = offline.release();
    backendKind = BackendKind::Compute;
}

AnyBox::Graphics::PipelineState::PipelineState(std::unique_ptr<OfflineGraphicsPipeline> graphics, Device* device, RootSignature* root) : vtable(&GetPipelineTable()), host(nullptr), device(device), root(root) {
    initialize();
    backend.graphics = graphics.release();
    backendKind = BackendKind::Graphics;
}

AnyBox::Graphics::PipelineState::PipelineState(std::unique_ptr<NativeComputePipeline> native, Device* device, RootSignature* root) : vtable(&GetPipelineTable()), host(nullptr), device(device), root(root) {
    initialize();
    backend.native = native.release();
    backendKind = BackendKind::NativeCompute;
}

void AnyBox::Graphics::PipelineState::initialize() {
    RootSignature::AddRef(root);
    try { Device::AddRef(device); } catch (...) { RootSignature::Release(root); throw; }
    auto address = reinterpret_cast<std::uintptr_t>(this);
    inlineCommands = {Commands::PipelineHeader, static_cast<std::uint32_t>(address), static_cast<std::uint32_t>(address >> 32), 0, 0, 0, 0, 0};
}

AnyBox::Graphics::PipelineState* AnyBox::Graphics::PipelineState::From(void* object, Device* owner) {
    if (!object) throw std::invalid_argument("Pipeline state is null");
    const ObjectTable* table;
    std::memcpy(&table, object, sizeof(table));
    if (table != &GetPipelineTable()) throw std::invalid_argument("Foreign pipeline state object is unsupported");
    auto* pipeline = static_cast<PipelineState*>(object);
    if (pipeline->device != owner) throw std::invalid_argument("Pipeline state and command list devices differ");
    return pipeline;
}

AnyBox::Graphics::PipelineState::~PipelineState() {
    privateData.Reset();
    if (backendKind == BackendKind::Compute) delete backend.offline;
    if (backendKind == BackendKind::Graphics) delete backend.graphics;
    if (backendKind == BackendKind::NativeCompute) delete backend.native;
    if (host) host->Release();
    RootSignature::Release(root);
    Device::Release(device);
}

HRESULT AnyBox::Graphics::PipelineState::GetCachedBlob(PipelineState*, ID3DBlob** blob) {
    if (!blob) throw std::invalid_argument("Pipeline cache blob output is null");
    *blob = nullptr;
    throw std::runtime_error("Xbox pipeline cache serialization is not implemented");
}

const AnyBox::Graphics::ObjectTable& AnyBox::Graphics::GetPipelineTable() {
    static const auto table = [] {
        auto value = MakeObjectTable<PipelineState>(std::make_index_sequence<std::tuple_size_v<ObjectTable>>{});
        value[8] = MethodAddress(&PipelineState::GetCachedBlob);
        return value;
    }();
    return table;
}
