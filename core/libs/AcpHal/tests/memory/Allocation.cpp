#include "Tests.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

void AnyBox::Tests::CheckAllocation() {
    std::array<void*, 3> allocations{};
    std::array<APU_ADDRESS, 3> physical{};
    for (std::uint32_t i = 0; i < allocations.size(); ++i) {
        Require(ApuAlloc(&allocations[i], &physical[i], 4096, 2048, 0) == 0, "APU allocation failed");
        Require(reinterpret_cast<std::uintptr_t>(allocations[i]) % 2048 == 0, "APU allocation is misaligned");
        Require(physical[i] != 0 && physical[i] % 2048 == 0, "APU physical output is misaligned");
        Require(ApuMapVirtualAddress(allocations[i]) == physical[i], "APU physical output does not match its mapping");
        std::memset(allocations[i], static_cast<int>(i + 1), 4096);
    }
    for (std::size_t i = 0; i < allocations.size(); ++i) {
        for (std::size_t j = i + 1; j < allocations.size(); ++j) Require(allocations[i] != allocations[j], "APU allocations overlap");
        auto* bytes = static_cast<const std::byte*>(allocations[i]);
        for (std::size_t j = 0; j < 4096; ++j) Require(bytes[j] == std::byte(i + 1), "APU allocation contents changed");
    }
    ApuFree(allocations[1]);
    void* replacement = nullptr;
    Require(ApuAlloc(&replacement, nullptr, 4096, 2048, 0) == 0, "APU allocation without physical output failed");
    Require(replacement != allocations[0] && replacement != allocations[2], "APU replacement overlaps a live allocation");
    ApuFree(allocations[0]);
    ApuFree(allocations[2]);
    ApuFree(replacement);

    void* nonCached = nullptr;
    APU_ADDRESS nonCachedPhysical = 0;
    Require(ApuAlloc(&nonCached, &nonCachedPhysical, 4096, 2048, 2) == 0, "Non-cached APU allocation failed");
    Require(nonCachedPhysical >= 0x10000 + ApuCachedSize, "Non-cached allocation used the cached partition");
    Require(ApuMapApuAddress(nonCachedPhysical) == nonCached, "Non-cached mapping did not round-trip");
    ApuFree(nonCached);

    void* entireCached = nullptr;
    Require(ApuAlloc(&entireCached, nullptr, ApuCachedSize, 4, 1) == 0, "Cached APU free blocks were not coalesced");
    std::memset(entireCached, 0x5A, ApuCachedSize);
    ApuFree(entireCached);
    void* entireNonCached = nullptr;
    Require(ApuAlloc(&entireNonCached, nullptr, ApuNonCachedSize, 4, 2) == 0, "Non-cached APU free blocks were not coalesced");
    std::memset(entireNonCached, 0xA5, ApuNonCachedSize);
    ApuFree(entireNonCached);

    ApuFree(nullptr);
}
