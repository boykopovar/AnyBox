#include "Fixture.hpp"

void CheckSerializedGraphicsBlend() {
    using namespace Fixtures;
    using namespace SerializedGraphicsTests;
    Fixture fixture;
    fixture.Context(116, 0x87654321);
    for (std::uint32_t index = 0; index < 8; ++index) {
        fixture.Context(120 + index * 4, 0xC0000000 | 4 | (5 << 8) | (6 << 16) | (7 << 24));
    }
    auto output = fixture.Read();
    for (std::size_t index = 0; index < 8; ++index) {
        const auto& target = output.targets[index];
        Require(target.enable && !target.logicEnable && target.writeMask == index + 1);
        Require(target.source == 5 && target.destination == 6 && target.sourceAlpha == 7 && target.destinationAlpha == 8);
        Require(target.operation == 1 && target.alphaOperation == 1);
    }
    for (std::uint32_t factor = 1; factor <= 19; ++factor) {
        fixture.Context(120, 0xC0000000 | (factor - 1));
        output = fixture.Read();
        Require(output.targets.front().source == factor && output.independentBlend == (factor < 16));
    }
    constexpr std::array<std::uint32_t, 5> encodings{0, 1, 4, 2, 3};
    for (std::uint32_t index = 0; index < encodings.size(); ++index) {
        fixture.Context(120, 0xC0000000 | (encodings[index] << 5) | (encodings[4 - index] << 21));
        output = fixture.Read();
        Require(output.targets.front().operation == index + 1 && output.targets.front().alphaOperation == 5 - index);
    }
    fixture.Context(120, 0x80000000);
    fixture.auxiliary[43] = std::byte{1};
    for (std::uint8_t operation = 0; operation < 16; ++operation) {
        fixture.auxiliary[45] = static_cast<std::byte>(operation);
        output = fixture.Read();
        Require(output.targets.front().logicEnable && !output.targets.front().enable && output.targets.front().logicOperation == operation);
    }
    fixture.Context(120, 0xC0000000);
    Reject<std::invalid_argument>([&] { (void)fixture.Read(); });
    fixture.auxiliary[43] = std::byte{};
    fixture.auxiliary[45] = std::byte{16};
    Reject<std::runtime_error>([&] { (void)fixture.Read(); });
    fixture.auxiliary[45] = std::byte{};
    for (auto control : {0xC000001Fu, 0xC0001F00u, 0xC01F0000u, 0xDF000000u, 0xC00000E0u, 0xC0E00000u}) {
        fixture.Context(120, control);
        Reject<std::runtime_error>([&] { (void)fixture.Read(); });
    }
}
