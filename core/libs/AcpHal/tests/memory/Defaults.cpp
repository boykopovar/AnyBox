#include "Tests.hpp"

#include <array>
#include <atomic>
#include <barrier>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <new>
#include <thread>

namespace {

constexpr std::uint32_t DefaultCachedSize = 64 * 1024 * 1024;
constexpr std::uint32_t DefaultNonCachedSize = 1024 * 1024;
std::atomic<std::uint32_t> backingAllocations = 0;
std::atomic<std::uint32_t> backingFrees = 0;

void* Allocate(std::size_t size, std::uint64_t attributes) {
    ++backingAllocations;
    AnyBox::Tests::Require(size == DefaultCachedSize + DefaultNonCachedSize, "Default APU heap size changed");
    AnyBox::Tests::Require(attributes == 0, "Default APU backing attributes changed");
    return static_cast<std::byte*>(::operator new(size + 16, std::align_val_t{0x10000})) + 16;
}

void Free(void* memory, std::uint64_t attributes) {
    AnyBox::Tests::Require(attributes == 0, "Default APU backing free attributes changed");
    ++backingFrees;
    ::operator delete(static_cast<std::byte*>(memory) - 16, std::align_val_t{0x10000});
}

}

struct TitleHooks {
    decltype(&Allocate) Allocation;
    decltype(&Free) Release;
};

extern "C" {
TitleHooks __XMemAllocationHooks__{Allocate, Free};
}

int main() {
    try {
        using namespace AnyBox::Tests;
        std::array<std::exception_ptr, 8> errors{};
        std::array<std::jthread, 8> threads;
        std::barrier start(static_cast<std::ptrdiff_t>(threads.size() + 1));
        for (std::size_t i = 0; i < threads.size(); ++i) {
            threads[i] = std::jthread([&errors, &start, i] {
                start.arrive_and_wait();
                try {
                    for (std::uint32_t j = 0; j < 32; ++j) {
                        void* allocation = nullptr;
                        APU_ADDRESS physical = 0;
                        ApuAlloc(&allocation, &physical, 2048, 2048, j % 3);
                        Require(physical == ApuMapVirtualAddress(allocation), "Default APU mapping changed");
                        Require(ApuMapApuAddress(physical) == allocation, "Default APU reverse mapping changed");
                        *static_cast<std::byte*>(allocation) = std::byte(i + 1);
                        ApuFree(allocation);
                    }
                } catch (...) {
                    errors[i] = std::current_exception();
                }
            });
        }
        start.arrive_and_wait();
        for (auto& thread : threads) thread.join();
        for (const auto& error : errors) if (error) std::rethrow_exception(error);
        Require(backingAllocations == 1, "Concurrent lazy APU creation allocated multiple heaps");
        ExpectException<std::logic_error>([] { ApuCreateHeap(1024 * 1024, 1024 * 1024); });
        void* cached = nullptr;
        Require(ApuAlloc(&cached, nullptr, DefaultCachedSize, 4, 0) == 0, "Default cached APU heap capacity changed");
        ApuFree(cached);
        void* nonCached = nullptr;
        Require(ApuAlloc(&nonCached, nullptr, DefaultNonCachedSize, 4, 2) == 0, "Default non-cached APU heap capacity changed");
        ApuFree(nonCached);
        Require(backingAllocations == 1 && backingFrees == 0, "Default APU heap backing lifetime changed");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
