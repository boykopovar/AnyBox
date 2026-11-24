#include "Tests.hpp"

#include <cstddef>
#include <cstdint>

void AnyBox::Tests::CheckMapping() {
    void* allocation = nullptr;
    APU_ADDRESS address = 0;
    Require(ApuAlloc(&allocation, &address, 65536, 65536, 0) == 0, "APU mapping allocation failed");
    Require(address != 0 && address % 65536 == 0, "APU bus alignment changed");
    auto* bytes = static_cast<std::byte*>(allocation);
    for (std::uint32_t i = 0; i < 65536; ++i) {
        Require(ApuMapVirtualAddress(bytes + i) == address + i, "APU translation is not affine");
        Require(ApuMapApuAddress(address + i) == bytes + i, "APU translation did not round-trip");
        Require(ApuIsVirtualAddressValid(bytes + i, 64) == ((address + i) % 64 == 0), "APU validity ignored physical alignment");
    }
    Require(ApuIsVirtualAddressValid(allocation, 0), "Zero physical alignment rejected a live APU allocation");
    *static_cast<std::byte*>(ApuMapApuAddress(address + 123)) = std::byte{0x7B};
    Require(bytes[123] == std::byte{0x7B}, "APU address did not reference CPU storage");
    ApuFree(allocation);
    Require(ApuMapVirtualAddress(allocation) == 0, "Freed virtual storage still has an APU mapping");
    Require(ApuMapApuAddress(address) == nullptr, "Freed APU storage still has a virtual mapping");
    Require(!ApuIsVirtualAddressValid(allocation, 0), "Freed virtual storage is still valid");
    Require(!ApuIsVirtualAddressValid(bytes + 123, 1), "Freed interior storage is still valid");
}
