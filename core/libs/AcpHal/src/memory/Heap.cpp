#include "Heap.hpp"

#include <xmem.hpp>

#include <limits>
#include <new>
#include <stdexcept>

namespace {

constexpr std::uint32_t DefaultCachedSize = 64 * 1024 * 1024;
constexpr std::uint32_t DefaultNonCachedSize = 1024 * 1024;
constexpr std::uint64_t BackingAttributes = 0;

}

void AnyBox::AcpHal::Heap::Create(std::uint32_t cachedSize, std::uint32_t nonCachedSize) {
    CreateInternal(cachedSize, nonCachedSize, false);
}

void AnyBox::AcpHal::Heap::EnsureCreated() {
    CreateInternal(DefaultCachedSize, DefaultNonCachedSize, true);
}

void AnyBox::AcpHal::Heap::CreateInternal(std::uint32_t cachedSize, std::uint32_t nonCachedSize, bool allowExisting) {
    if (cachedSize == 0) throw std::invalid_argument("APU cached heap size must be nonzero");
    if (nonCachedSize == 0) nonCachedSize = DefaultNonCachedSize;
    auto totalSize = static_cast<std::uint64_t>(cachedSize) + nonCachedSize;
    if (totalSize > 0xFFFF0000) throw std::length_error("APU heap exceeds its 32-bit address space");

    std::unique_lock lock(mutex);
    if (state) {
        if (allowExisting) return;
        throw std::logic_error("APU heap already exists");
    }
    while (creating) {
        if (creatingThread == std::this_thread::get_id()) throw std::logic_error("APU heap creation is reentrant");
        condition.wait(lock);
    }
    if (state) {
        if (allowExisting) return;
        throw std::logic_error("APU heap already exists");
    }
    creating = true;
    creatingThread = std::this_thread::get_id();
    lock.unlock();

    std::shared_ptr<State> created;
    try {
        created = CreateState(cachedSize, nonCachedSize);
    } catch (...) {
        lock.lock();
        creating = false;
        creatingThread = {};
        lock.unlock();
        condition.notify_all();
        throw;
    }

    lock.lock();
    state = std::move(created);
    creating = false;
    creatingThread = {};
    lock.unlock();
    condition.notify_all();
}

std::shared_ptr<AnyBox::AcpHal::Heap::State> AnyBox::AcpHal::Heap::CreateState(std::uint32_t cachedSize, std::uint32_t nonCachedSize) {
    auto created = std::make_shared<State>();
    created->cachedBlocks.push_back({0, cachedSize, false});
    created->nonCachedBlocks.push_back({cachedSize, nonCachedSize, false});

    auto totalSize = static_cast<std::uint64_t>(cachedSize) + nonCachedSize;
    auto* allocation = static_cast<std::byte*>(XMemAlloc(static_cast<std::size_t>(totalSize), BackingAttributes));
    if (!allocation) throw std::bad_alloc();
    auto address = reinterpret_cast<std::uintptr_t>(allocation);
    if (totalSize > std::numeric_limits<std::uintptr_t>::max() - address) {
        XMemFree(allocation, BackingAttributes);
        throw std::overflow_error("APU heap CPU address range overflows");
    }
    auto busAddress = 0x10000 + static_cast<std::uint32_t>(address & 0xFFFF);
    if (busAddress + totalSize > 0x100000000) {
        XMemFree(allocation, BackingAttributes);
        throw std::length_error("APU heap bus address range overflows");
    }

    created->memory = allocation;
    created->memorySize = static_cast<std::uint32_t>(totalSize);
    created->busBase = busAddress;
    return created;
}

AnyBox::AcpHal::Heap& AnyBox::AcpHal::GetHeap() {
    static Heap heap;
    return heap;
}
