#include "Tests.hpp"

#include <array>
#include <cstddef>
#include <limits>
#include <new>

void AnyBox::Tests::CheckErrors() {
    std::array<std::byte, 64> foreign{};
    Require(!ApuIsVirtualAddressValid(foreign.data(), 1), "Foreign storage was accepted as APU memory");
    Require(ApuMapVirtualAddress(foreign.data()) == 0, "Foreign virtual storage received an APU mapping");
    Require(ApuMapApuAddress(1) == nullptr, "Invalid low APU address received a virtual mapping");
    Require(ApuMapApuAddress(std::numeric_limits<APU_ADDRESS>::max()) == nullptr, "Invalid high APU address received a virtual mapping");

    APU_ADDRESS physical = std::numeric_limits<APU_ADDRESS>::max();
    ExpectException<std::invalid_argument>([&] { ApuAlloc(nullptr, &physical, 64, 64, 0); });
    Require(physical == 0, "Null virtual output did not clear the physical output");
    void* output = foreign.data();
    auto expectInvalid = [&](auto&& callable) {
        output = foreign.data();
        physical = std::numeric_limits<APU_ADDRESS>::max();
        ExpectException<std::invalid_argument>(callable);
        Require(output == nullptr && physical == 0, "Failed APU allocation did not clear its outputs");
    };
    expectInvalid([&] { ApuAlloc(&output, &physical, 0, 64, 0); });
    expectInvalid([&] { ApuAlloc(&output, &physical, 64, 0, 0); });
    expectInvalid([&] { ApuAlloc(&output, &physical, 64, 2, 0); });
    expectInvalid([&] { ApuAlloc(&output, &physical, 64, 3, 0); });
    expectInvalid([&] { ApuAlloc(&output, &physical, 64, 0x20000, 0); });
    expectInvalid([&] { ApuAlloc(&output, &physical, 64, 64, 3); });
    output = foreign.data();
    physical = std::numeric_limits<APU_ADDRESS>::max();
    ExpectException<std::bad_alloc>([&] { ApuAlloc(&output, &physical, ApuCachedSize + 1, 4, 0); });
    Require(output == nullptr && physical == 0, "Exhausted APU allocation did not clear its outputs");

    void* allocation = nullptr;
    Require(ApuAlloc(&allocation, nullptr, 64, 64, 0) == 0, "Valid APU allocation failed");
    ExpectException<std::invalid_argument>([&] { ApuFree(static_cast<std::byte*>(allocation) + 1); });
    ApuFree(allocation);
    ExpectException<std::invalid_argument>([&] { ApuFree(allocation); });
    ExpectException<std::invalid_argument>([&] { ApuFree(foreign.data()); });
}
