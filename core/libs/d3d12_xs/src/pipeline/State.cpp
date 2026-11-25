#include "State.hpp"
#include "Offline.hpp"
#include <anybox/graphics/commands/Inline.hpp>
#include <mutex>
#include <unordered_map>

namespace {
std::mutex pipelineMutex;
std::unordered_map<std::uint64_t, AnyBox::Graphics::PipelineState*> preparedPipelines;
}

AnyBox::Graphics::PipelineState::PipelineState(ID3D12PipelineState* host, Device* device, RootSignature* root) : vtable(&GetPipelineTable()), host(host), device(device), root(root) {
    initialize();
}

AnyBox::Graphics::PipelineState::PipelineState(std::unique_ptr<OfflinePipeline> offline, Device* device, RootSignature* root) : vtable(&GetPipelineTable()), host(nullptr), device(device), root(root), offline(offline.get()) {
    initialize();
    offline.release();
}

void AnyBox::Graphics::PipelineState::initialize() {
    RootSignature::AddRef(root);
    try { Device::AddRef(device); } catch (...) { RootSignature::Release(root); throw; }
    auto address = reinterpret_cast<std::uintptr_t>(this);
    inlineCommands = {Commands::PipelineBindHeader, static_cast<std::uint32_t>(address), static_cast<std::uint32_t>(address >> 32), 0, 0, 0, 0, 0};
    if (!offline) return;
    try {
        std::lock_guard lock(pipelineMutex);
        preparedPipelines.emplace(address, this);
    } catch (...) {
        Device::Release(device);
        RootSignature::Release(root);
        throw;
    }
}

std::shared_ptr<AnyBox::Graphics::PipelineState> AnyBox::Graphics::PipelineState::Acquire(std::uint64_t address, Device* device) {
    PipelineState* pipeline;
    {
        std::lock_guard lock(pipelineMutex);
        auto found = preparedPipelines.find(address);
        if (found == preparedPipelines.end() || found->second->device != device) throw std::invalid_argument("Inline compute pipeline is not a prepared pipeline on this device");
        pipeline = found->second;
        AddRef(pipeline);
    }
    return std::shared_ptr<PipelineState>(pipeline, &PipelineState::Release);
}

std::uint32_t AnyBox::Graphics::PipelineState::Release(PipelineState* self) {
    if (!self->offline) return ObjectLifetime::Release(self);
    std::uint32_t previous;
    {
        std::lock_guard lock(pipelineMutex);
        previous = self->references.fetch_sub(1, std::memory_order_acq_rel);
        if (previous == 1) preparedPipelines.erase(reinterpret_cast<std::uintptr_t>(self));
    }
    if (previous == 0) throw std::logic_error("Graphics object reference count underflows");
    if (previous == 1) delete self;
    return previous - 1;
}

AnyBox::Graphics::PipelineState::~PipelineState() {
    privateData.Reset();
    delete offline;
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
