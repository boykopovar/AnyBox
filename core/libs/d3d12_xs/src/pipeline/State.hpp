#pragma once

#include "../root/Signature.hpp"
#include "../object/ManagedProperties.hpp"

namespace AnyBox::Graphics {
inline constexpr Guid PipelineStateId{0x765A30F3, 0xF624, 0x4C6F, {0xA8, 0x28, 0xAC, 0xE9, 0x48, 0x62, 0x24, 0x45}};
const ObjectTable& GetPipelineTable();
class OfflinePipeline;

class alignas(16) PipelineState : public ObjectLifetime<PipelineState, ManagedProperties> {
public:
    PipelineState(ID3D12PipelineState* host, Device* device, RootSignature* root);
    PipelineState(std::unique_ptr<OfflinePipeline> offline, Device* device, RootSignature* root);
    ~PipelineState();
    ID3D12PipelineState* GetHost() const {
        if (!host) throw std::logic_error("Offline Vulkan pipeline has no host D3D12 pipeline state");
        return host;
    }
    const OfflinePipeline* GetOffline() const noexcept { return offline; }
    PrivateData& GetPrivateDataStore() { return privateData.Get(); }
    Device* GetParent() const noexcept { return device; }
    RootSignature* GetRootSignature() const noexcept { return root; }
    static std::shared_ptr<PipelineState> Acquire(std::uint64_t address, Device* device);
    static std::uint32_t Release(PipelineState* self);
    static bool Supports(const Guid& interfaceId) noexcept { return interfaceId == PipelineStateId || interfaceId == UnknownId || interfaceId == ObjectId || interfaceId == DeviceChildId || interfaceId == PageableId; }
    static HRESULT GetCachedBlob(PipelineState* self, ID3DBlob** blob);
    static constexpr std::size_t GetVtableOffset() { return offsetof(PipelineState, vtable); }
    static constexpr std::size_t GetInlineOffset() { return offsetof(PipelineState, inlineCommands); }

private:
    friend class ObjectLifetime<PipelineState, AnyBox::Graphics::ManagedProperties>;
    std::atomic<std::uint32_t>& getReferences() noexcept { return references; }
    const ObjectTable* vtable;
    [[maybe_unused]] std::uint32_t objectFlags = 0;
    std::atomic<std::uint32_t> references{1};
    std::array<std::uint32_t, 8> inlineCommands{};
    ID3D12PipelineState* host;
    Device* device;
    RootSignature* root;
    OfflinePipeline* offline = nullptr;
    PrivateDataSlot privateData;
    [[maybe_unused]] std::uint64_t reserved = 0;
    void initialize();
};
static_assert(std::is_standard_layout_v<PipelineState>);
static_assert(PipelineState::GetVtableOffset() == 0);
static_assert(PipelineState::GetInlineOffset() == 16);
}
