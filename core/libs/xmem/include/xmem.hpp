#ifndef ANYBOX_XMEM_HPP
#define ANYBOX_XMEM_HPP

#include <xmemTypes.hpp>

#include <cstddef>
#include <cstdint>

extern "C" {

void* XMemAlloc(std::size_t size, std::uint64_t attributes) noexcept(false);
void* XMemAllocDefault(std::size_t size, std::uint64_t attributes, void* context, const XMemAllocationHook* nextHook) noexcept(false);
std::int32_t XMemAllocatePhysicalPages(std::uint64_t attributes, std::size_t* pageCount, std::uint64_t* pageFrames) noexcept(false);
void XMemFree(void* allocation, std::uint64_t attributes) noexcept(false);
void XMemFreeDefault(void* allocation, std::uint64_t attributes, void* context, const XMemFreeHook* nextHook) noexcept(false);
std::int32_t XMemFreePhysicalPages(std::size_t pageCount, const std::uint64_t* pageFrames) noexcept(false);
std::int32_t XMemGetWorkingSetStatistics(std::uint32_t flags, XMemWorkingSetStatistics* statistics) noexcept(false);
std::int32_t XMemIsToolsMemAvailable() noexcept(false);
void* XMemMapPhysicalPages(void* address, std::size_t pageCount, const std::uint64_t* pageFrames) noexcept(false);
std::int32_t XMemMapPhysicalPagesScatter(void* const* addresses, std::size_t pageCount, const std::uint64_t* pageFrames) noexcept(false);
std::int32_t XMemTransferMemory(std::uint32_t memoryPool, std::size_t size) noexcept(false);
void* XMemVirtualAlloc(void* address, std::size_t size, std::uint32_t allocationType, std::uint64_t attributes, std::uint32_t protection) noexcept(false);
std::int32_t XMemVirtualQuery(void* process, const void* address, XMemMemoryBasicInformation* information) noexcept(false);

}

#endif
