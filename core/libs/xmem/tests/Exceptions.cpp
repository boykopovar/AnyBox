#include <xmem.hpp>
#include <anybox/ExceptionTest.hpp>

#include <type_traits>

static_assert(std::is_same_v<decltype(&XMemAllocatePhysicalPages), std::int32_t(*)(std::uint64_t, std::size_t*, std::uint64_t*)>);
static_assert(std::is_same_v<decltype(&XMemFreePhysicalPages), std::int32_t(*)(std::size_t, const std::uint64_t*)>);
static_assert(std::is_same_v<decltype(&XMemVirtualAlloc), void*(*)(void*, std::size_t, std::uint32_t, std::uint64_t, std::uint32_t)>);

int main() {
    return AnyBox::Tests::Run([] {
        std::size_t pageCount = 1;
        std::uint64_t pageFrame = 0x123456789ABCDEF0;
        void* address = nullptr;
        AnyBox::Tests::ExpectUnimplemented("XMemAllocDefault", [&] { XMemAlloc(4096, 0); });
        AnyBox::Tests::ExpectUnimplemented("XMemAllocDefault", [&] { XMemAllocDefault(4096, 0, nullptr, nullptr); });
        AnyBox::Tests::ExpectUnimplemented("XMemAllocatePhysicalPages", [&] { XMemAllocatePhysicalPages(0, &pageCount, &pageFrame); });
        AnyBox::Tests::ExpectUnimplemented("XMemFreeDefault", [&] { XMemFree(nullptr, 0); });
        AnyBox::Tests::ExpectUnimplemented("XMemFreeDefault", [&] { XMemFreeDefault(nullptr, 0, nullptr, nullptr); });
        AnyBox::Tests::ExpectUnimplemented("XMemFreePhysicalPages", [&] { XMemFreePhysicalPages(1, &pageFrame); });
        AnyBox::Tests::ExpectUnimplemented("XMemGetWorkingSetStatistics", [&] { XMemGetWorkingSetStatistics(0, nullptr); });
        if (XMemIsToolsMemAvailable() != 0) throw std::runtime_error("Development-kit tools memory is not available");
        AnyBox::Tests::ExpectUnimplemented("XMemMapPhysicalPages", [&] { XMemMapPhysicalPages(nullptr, 1, &pageFrame); });
        AnyBox::Tests::ExpectUnimplemented("XMemMapPhysicalPagesScatter", [&] { XMemMapPhysicalPagesScatter(&address, 1, &pageFrame); });
        AnyBox::Tests::ExpectUnimplemented("XMemTransferMemory", [&] { XMemTransferMemory(1, 4096); });
        AnyBox::Tests::ExpectUnimplemented("XMemVirtualAlloc", [&] { XMemVirtualAlloc(nullptr, 4096, 0, 0, 0); });
        AnyBox::Tests::ExpectUnimplemented("XMemVirtualQuery", [] { XMemVirtualQuery(nullptr, nullptr, nullptr); });
        if (pageCount != 1 || pageFrame != 0x123456789ABCDEF0 || address != nullptr) throw std::runtime_error("An unimplemented export changed its output");
    });
}
