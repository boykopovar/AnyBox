#pragma once

#include "../Fixtures.hpp"
#include <anybox/graphics/container/SerializedGraphicsState.hpp>
#include <array>
#include <bit>

namespace SerializedGraphicsTests {
struct Fixture {
    std::array<std::byte, 388> state{};
    std::array<std::byte, 80> auxiliary{};

    Fixture() {
        Fixtures::Set(state, 0, 0x4005D808);
        Fixtures::Set(state, 4, 4);
        Fixtures::Set(state, 8, 0x100060);
        Fixtures::Set(state, 16, 0xB50516A0);
        Fixtures::Set(state, 40, (76 << 16) | 284);
        Fixtures::Set(state, 88 + 72, 2 << 8);
        Fixtures::Set(state, 88 + 104, 0x00FFFF00);
        Fixtures::Set(state, 88 + 108, 0x00FFFF00);
        Fixtures::Set(auxiliary, 4, 4096);
        Fixtures::Set(auxiliary, 16, 28);
        Fixtures::Set(auxiliary, 28, UINT32_MAX);
        auxiliary[37] = std::byte{1};
        auxiliary[38] = std::byte{3};
        auxiliary[40] = std::byte{1};
    }

    void Context(std::size_t offset, std::uint32_t value) { Fixtures::Set(state, 88 + offset, value); }
    void ContextFloat(std::size_t offset, float value) { Context(offset, std::bit_cast<std::uint32_t>(value)); }
    AnyBox::Graphics::Container::SerializedGraphicsFixedState Read() const { return AnyBox::Graphics::Container::ReadSerializedGraphicsFixedState(state, auxiliary); }
};
}
