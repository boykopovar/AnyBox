#include <xmem.hpp>
#include <anybox/Unimplemented.hpp>

void* XMemAllocDefault(std::size_t, std::uint64_t, void*, const XMemAllocationHook*) noexcept(false) {
    ANYBOX_NOT_IMPLEMENTED();
}

std::int32_t XMemAllocatePhysicalPages(std::uint64_t, std::size_t*, std::uint64_t*) noexcept(false) {
    ANYBOX_NOT_IMPLEMENTED();
}

void XMemFreeDefault(void*, std::uint64_t, void*, const XMemFreeHook*) noexcept(false) {
    ANYBOX_NOT_IMPLEMENTED();
}

std::int32_t XMemFreePhysicalPages(std::size_t, const std::uint64_t*) noexcept(false) {
    ANYBOX_NOT_IMPLEMENTED();
}

std::int32_t XMemGetWorkingSetStatistics(std::uint32_t, XMemWorkingSetStatistics*) noexcept(false) {
    ANYBOX_NOT_IMPLEMENTED();
}

void* XMemMapPhysicalPages(void*, std::size_t, const std::uint64_t*) noexcept(false) {
    ANYBOX_NOT_IMPLEMENTED();
}

std::int32_t XMemMapPhysicalPagesScatter(void* const*, std::size_t, const std::uint64_t*) noexcept(false) {
    ANYBOX_NOT_IMPLEMENTED();
}

std::int32_t XMemTransferMemory(std::uint32_t, std::size_t) noexcept(false) {
    ANYBOX_NOT_IMPLEMENTED();
}

void* XMemVirtualAlloc(void*, std::size_t, std::uint32_t, std::uint64_t, std::uint32_t) noexcept(false) {
    ANYBOX_NOT_IMPLEMENTED();
}

std::int32_t XMemVirtualQuery(void*, const void*, XMemMemoryBasicInformation*) noexcept(false) {
    ANYBOX_NOT_IMPLEMENTED();
}
