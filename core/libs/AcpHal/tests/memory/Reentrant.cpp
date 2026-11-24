#include "Tests.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <new>

namespace {

constexpr std::uint32_t DefaultHeapSize = 65 * 1024 * 1024;
std::uint32_t backingAllocations = 0;
std::uint32_t backingFrees = 0;
bool reentered = false;

void* Allocate(std::size_t size, std::uint64_t attributes) {
    ++backingAllocations;
    AnyBox::Tests::Require(size == DefaultHeapSize && attributes == 0, "Reentrant APU backing arguments changed");
    if (!reentered) {
        reentered = true;
        void* nested = reinterpret_cast<void*>(1);
        APU_ADDRESS physical = 0xFFFFFFFF;
        AnyBox::Tests::ExpectException<std::logic_error>([&] { ApuAlloc(&nested, &physical, 64, 64, 0); });
        AnyBox::Tests::Require(nested == nullptr && physical == 0, "Reentrant APU failure did not clear its outputs");
    }
    return static_cast<std::byte*>(::operator new(size + 16, std::align_val_t{0x10000})) + 16;
}

void Free(void* memory, std::uint64_t attributes) {
    AnyBox::Tests::Require(attributes == 0, "Reentrant APU backing free attributes changed");
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
        void* allocation = nullptr;
        APU_ADDRESS physical = 0;
        Require(ApuAlloc(&allocation, &physical, 64, 64, 0) == 0, "Reentrant title hook prevented outer APU allocation");
        Require(reentered && backingAllocations == 1, "Reentrant title hook recursively initialized the heap");
        Require(physical == ApuMapVirtualAddress(allocation), "Outer APU allocation was not published");
        ApuFree(allocation);
        Require(backingFrees == 0, "Reentrant APU heap was released prematurely");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
