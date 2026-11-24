#include "Heap.hpp"

#include <AcpHal.hpp>

#include <stdexcept>

std::int32_t ApuAlloc(void** virtualAddress, APU_ADDRESS* physicalAddress, std::uint32_t size, std::uint32_t alignment, std::uint32_t flags) noexcept(false) {
    if (physicalAddress) *physicalAddress = 0;
    if (!virtualAddress) throw std::invalid_argument("APU virtual address output is null");
    *virtualAddress = nullptr;
    auto& heap = AnyBox::AcpHal::GetHeap();
    auto* result = heap.Allocate(size, alignment, flags);
    auto apuAddress = heap.ToApu(result);
    *virtualAddress = result;
    if (physicalAddress) *physicalAddress = apuAddress;
    return 0;
}

std::int32_t ApuCreateHeap(std::uint32_t cachedSize, std::uint32_t nonCachedSize) noexcept(false) {
    AnyBox::AcpHal::GetHeap().Create(cachedSize, nonCachedSize);
    return 0;
}

void ApuFree(void* allocation) noexcept(false) {
    AnyBox::AcpHal::GetHeap().Free(allocation);
}

bool ApuIsVirtualAddressValid(const void* address, std::uint32_t physicalAlignment) noexcept(false) {
    return AnyBox::AcpHal::GetHeap().IsValid(address, physicalAlignment);
}

void* ApuMapApuAddress(APU_ADDRESS apuAddress) noexcept(false) {
    return AnyBox::AcpHal::GetHeap().ToVirtual(apuAddress);
}

APU_ADDRESS ApuMapVirtualAddress(const void* address) noexcept(false) {
    return AnyBox::AcpHal::GetHeap().ToApu(address);
}
