#pragma once

#include "Backing.hpp"
#include "Texture.hpp"
#include "../object/Lifetime.hpp"
#include "../object/ManagedProperties.hpp"
#include <memory>

namespace AnyBox::Graphics {
inline constexpr Guid ResourceId{0x696442BE, 0xA72E, 0x4059, {0xBC, 0x79, 0x5B, 0x5C, 0x98, 0x04, 0x0F, 0xAD}};
inline constexpr Guid Resource1Id{0x9D5E227A, 0x4430, 0x4161, {0x88, 0xB3, 0x3E, 0xCA, 0x6B, 0xB1, 0x6E, 0x19}};
inline constexpr Guid Resource2Id{0xBE36EC3B, 0xEA85, 0x4AEB, {0xA4, 0x5A, 0xE9, 0xD7, 0x64, 0x04, 0xA4, 0x95}};
const ObjectTable& GetResourceTable();

class Resource : public ObjectLifetime<Resource, ManagedProperties> {
public:
    Resource(BufferBacking backing, Device* device, std::uintptr_t address, const D3D12_RESOURCE_DESC1& description);
    Resource(std::unique_ptr<TextureBacking> backing, Device* device, const D3D12_RESOURCE_DESC1& description);
    ~Resource();
    Vulkan::Buffer& GetGpuBuffer() const {
        if (!buffer) throw std::logic_error("Texture resource has no Vulkan buffer");
        return *buffer;
    }
    Vulkan::Image& GetGpuImage() const {
        if (!texture) throw std::logic_error("Buffer resource has no Vulkan image");
        return texture->allocation.GetImage();
    }
    Texture::Resource& GetTexture() const {
        if (!texture) throw std::logic_error("Buffer resource has no texture backing");
        return texture->allocation;
    }
    Descriptor::SurfacePitch GetTexturePitch() const {
        if (!texture) throw std::logic_error("Buffer resource has no texture pitch");
        return texture->pitch;
    }
    static bool Supports(const Guid& interfaceId) noexcept;
    static std::uint32_t Release(Resource* self);
    static std::shared_ptr<Resource> AcquirePlacedBuffer(Device* device, std::uint64_t address);
    static Resource* From(void* value, Device* owner);
    const D3D12_RESOURCE_DESC1& GetMetadata() const noexcept { return description; }
    static constexpr std::size_t GetVtableOffset() { return offsetof(Resource, vtable); }
    static HRESULT Map(Resource* self, std::uint32_t subresource, const D3D12_RANGE* range, void** data);
    static void Unmap(Resource* self, std::uint32_t subresource, const D3D12_RANGE* range);
    static D3D12_RESOURCE_DESC GetDescription(Resource* self);
    static D3D12_RESOURCE_DESC1 GetDescription1(Resource* self);
    static std::uint64_t GetGPUVirtualAddress(Resource* self) noexcept;
    static HRESULT GetHeapProperties(Resource* self, D3D12_HEAP_PROPERTIES* properties, D3D12_HEAP_FLAGS* flags);
    PrivateData& GetPrivateDataStore() { return privateData.Get(); }

private:
    friend class ObjectLifetime<Resource, AnyBox::Graphics::ManagedProperties>;
    std::atomic<std::uint32_t>& getReferences() noexcept { return references; }
    const ObjectTable* vtable;
    std::uint32_t objectFlags = 0;
    std::atomic<std::uint32_t> references{1};
    Vulkan::Buffer* buffer;
    Device* device;
    IPhysicalPageLease* pages;
    std::uintptr_t address;
    D3D12_RESOURCE_DESC1 description;
    PrivateDataSlot privateData;
    TextureBacking* texture = nullptr;
};
static_assert(std::is_standard_layout_v<Resource>);
static_assert(Resource::GetVtableOffset() == 0);
}
