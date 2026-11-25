#include "Resource.hpp"
#include "Texture.hpp"
#include "../object/PrivateData.hpp"
#include <algorithm>
#include <mutex>
#include <unordered_map>

namespace {
using AnyBox::Graphics::Device;
using AnyBox::Graphics::Resource;

struct PlacedBuffers {
    std::mutex mutex;
    std::unordered_multimap<Device*, Resource*> entries;
};

PlacedBuffers& GetPlacedBuffers() {
    static PlacedBuffers buffers;
    return buffers;
}
}

AnyBox::Graphics::Resource::Resource(BufferBacking backing, Device* device, std::uintptr_t address, const D3D12_RESOURCE_DESC1& description) : vtable(&GetResourceTable()), buffer(backing.resource.get()), device(device), pages(backing.pages.get()), address(address), description(description) {
    Device::AddRef(device);
    try {
        auto& buffers = GetPlacedBuffers();
        std::lock_guard lock(buffers.mutex);
        buffers.entries.emplace(device, this);
        backing.resource.release();
        backing.pages.release();
    } catch (...) {
        Device::Release(device);
        throw;
    }
}

AnyBox::Graphics::Resource::Resource(std::unique_ptr<TextureBacking> backing, Device* device, const D3D12_RESOURCE_DESC1& description) : vtable(&GetResourceTable()), buffer(nullptr), device(device), pages(nullptr), address(0), description(description), texture(backing.get()) {
    Device::AddRef(device);
    backing.release();
}

AnyBox::Graphics::Resource::~Resource() {
    privateData.Reset();
    delete buffer;
    delete texture;
    if (pages) pages->Release();
    Device::Release(device);
}

std::uint32_t AnyBox::Graphics::Resource::Release(Resource* self) {
    if (!self->buffer) return ObjectLifetime::Release(self);
    std::uint32_t previous;
    {
        auto& buffers = GetPlacedBuffers();
        std::lock_guard lock(buffers.mutex);
        previous = self->references.fetch_sub(1, std::memory_order_acq_rel);
        if (previous == 0) throw std::logic_error("Graphics object reference count underflows");
        if (previous == 1) {
            auto [first, end] = buffers.entries.equal_range(self->device);
            auto entry = std::find_if(first, end, [self](const auto& value) { return value.second == self; });
            buffers.entries.erase(entry);
        }
    }
    if (previous == 1) delete self;
    return previous - 1;
}

std::shared_ptr<Resource> AnyBox::Graphics::Resource::AcquirePlacedBuffer(Device* device, std::uint64_t address) {
    Resource* result = nullptr;
    {
        auto& buffers = GetPlacedBuffers();
        std::lock_guard lock(buffers.mutex);
        auto [first, end] = buffers.entries.equal_range(device);
        for (; first != end; ++first) {
            auto* resource = first->second;
            if (address < resource->address || address - resource->address >= resource->description.Width) continue;
            if (result) throw std::invalid_argument("Prepared compute root address matches multiple placed buffers");
            result = resource;
        }
        if (!result) throw std::invalid_argument("Prepared compute root address does not match a live placed buffer");
        AddRef(result);
    }
    return std::shared_ptr<Resource>(result, &Resource::Release);
}

bool AnyBox::Graphics::Resource::Supports(const Guid& id) noexcept { return id == ResourceId || id == Resource1Id || id == Resource2Id || id == UnknownId || id == ObjectId || id == DeviceChildId || id == PageableId; }
