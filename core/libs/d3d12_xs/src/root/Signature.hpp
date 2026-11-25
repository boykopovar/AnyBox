#pragma once

#include "Description.hpp"
#include "Container.hpp"
#include "../object/Lifetime.hpp"
#include "../object/Table.hpp"
#include <memory>

namespace AnyBox::Graphics {
inline constexpr Guid RootSignatureId{0xC54A6B66, 0x72DF, 0x4EE8, {0x8B, 0xE5, 0xA9, 0x46, 0xA1, 0x42, 0x92, 0x14}};
class RootSignature : public ObjectLifetime<RootSignature> {
public:
    RootSignature(ID3D12RootSignature* host, Device* device, std::unique_ptr<RootDescription> layout) : vtable(&GetObjectTable<RootSignature>()), host(host), device(device), layout(layout.get()) {
        Device::AddRef(device);
        layout.release();
    }
    ~RootSignature() {
        host->Release();
        delete layout;
        Device::Release(device);
    }
    ID3D12RootSignature* GetHost() const noexcept { return host; }
    ID3D12RootSignature* GetHostForExecution() const {
        if ((layout->GetFlags() & XboxRootSignatureFlags) != 0) throw std::runtime_error("Xbox root signature register layout requires shader translation before execution");
        return host;
    }
    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& GetDescription() const noexcept { return layout->GetDescription(); }
    std::uint32_t GetFlags() const noexcept { return layout->GetFlags(); }
    const Container::RootSignatureLayout& GetLayout() const noexcept { return layout->GetLayout(); }
    const Shader::ShaderDigest& GetSourceDigest() const noexcept { return layout->GetSourceDigest(); }
    Device* GetParent() const noexcept { return device; }
    static bool Supports(const Guid& interfaceId) noexcept { return interfaceId == RootSignatureId || interfaceId == UnknownId || interfaceId == ObjectId || interfaceId == DeviceChildId; }
    static constexpr std::size_t GetVtableOffset() { return offsetof(RootSignature, vtable); }
    static RootSignature* From(void* object, Device* owner) {
        if (!object) throw std::invalid_argument("Root signature object is null");
        const ObjectTable* table;
        std::memcpy(&table, object, sizeof(table));
        if (table != &GetObjectTable<RootSignature>()) throw std::invalid_argument("Foreign root signature object is unsupported");
        auto* signature = static_cast<RootSignature*>(object);
        if (signature->device != owner) throw std::invalid_argument("Root signature and pipeline devices differ");
        return signature;
    }

private:
    friend class ObjectLifetime<RootSignature>;
    std::atomic<std::uint32_t>& getReferences() noexcept { return references; }
    const ObjectTable* vtable;
    std::atomic<std::uint32_t> references{1};
    ID3D12RootSignature* host;
    Device* device;
    RootDescription* layout;
};
static_assert(std::is_standard_layout_v<RootSignature>);
static_assert(RootSignature::GetVtableOffset() == 0);
}
