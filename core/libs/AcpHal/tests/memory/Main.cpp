#include "Tests.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <new>

namespace {

std::uint32_t backingAllocations = 0;
std::uint32_t backingFrees = 0;
bool failBackingAllocation = false;

void* Allocate(std::size_t size, std::uint64_t attributes) {
    ++backingAllocations;
    AnyBox::Tests::Require(size == AnyBox::Tests::ApuCachedSize + AnyBox::Tests::ApuNonCachedSize, "APU backing size ignored either heap partition");
    AnyBox::Tests::Require(attributes == 0, "APU backing allocation attributes changed");
    if (failBackingAllocation) {
        failBackingAllocation = false;
        return nullptr;
    }
    return static_cast<std::byte*>(::operator new(size + 16, std::align_val_t{0x10000})) + 16;
}

void Free(void* memory, std::uint64_t attributes) {
    AnyBox::Tests::Require(attributes == 0, "APU backing free attributes changed");
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
        Require(!ApuIsVirtualAddressValid(nullptr, 64), "Null APU address is valid");
        Require(ApuMapVirtualAddress(nullptr) == 0 && ApuMapApuAddress(0) == nullptr, "Null APU translations changed");
        Require(backingAllocations == 0, "Null APU queries initialized the heap");
        ExpectException<std::invalid_argument>([] { ApuCreateHeap(0, 0); });
        Require(backingAllocations == 0, "Rejected APU heap allocated storage");
        failBackingAllocation = true;
        ExpectException<std::bad_alloc>([] { ApuCreateHeap(ApuCachedSize, ApuNonCachedSize); });
        Require(backingAllocations == 1 && backingFrees == 0, "Failed APU backing allocation was not contained");
        Require(ApuCreateHeap(ApuCachedSize, ApuNonCachedSize) == 0, "APU heap creation did not recover after failure");
        Require(backingAllocations == 2, "APU heap did not commit both partitions in one backing region");
        ExpectException<std::logic_error>([] { ApuCreateHeap(ApuCachedSize, ApuNonCachedSize); });
        Require(backingAllocations == 2, "Duplicate APU heap creation allocated storage");
        CheckAllocation();
        CheckMapping();
        CheckErrors();
        CheckConcurrency();
        Require(backingAllocations == 2 && backingFrees == 0, "APU heap backing lifetime changed");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
