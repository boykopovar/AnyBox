#ifndef ANYBOX_ACPHAL_HPP
#define ANYBOX_ACPHAL_HPP

#include <AcpHalTypes.hpp>

#include <cstdint>

extern "C" {

std::int32_t AcpHalAllocateShapeContexts(AcpHalShapeContexts* shapeContexts) noexcept(false);
std::int32_t AcpHalCreate(IAcpHal** acpHal) noexcept(false);
void AcpHalReleaseShapeContexts() noexcept(false);
std::int32_t ApuAlloc(void** virtualAddress, APU_ADDRESS* physicalAddress, std::uint32_t size, std::uint32_t alignment, std::uint32_t flags) noexcept(false);
std::int32_t ApuCreateHeap(std::uint32_t cachedSize, std::uint32_t nonCachedSize) noexcept(false);
void ApuFree(void* allocation) noexcept(false);
bool ApuIsVirtualAddressValid(const void* address, std::uint32_t physicalAlignment) noexcept(false);
void* ApuMapApuAddress(APU_ADDRESS apuAddress) noexcept(false);
APU_ADDRESS ApuMapVirtualAddress(const void* address) noexcept(false);

}

#endif
