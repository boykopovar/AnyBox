#pragma once

#include <anybox/shader/SerializedGraphicsCompiler.hpp>
#include "../../../graphics/container/tests/graphics/Fixture.hpp"

namespace SerializedGraphicsTests {
struct StageFixture {
    Fixture graphics;
    std::array<std::byte, 128> code{};
    std::array<std::byte, 48> metadata{};

    explicit StageFixture(bool pixel) {
        using Fixtures::Set;
        for (std::size_t offset = 0; offset < 36; offset += 12) {
            Set(code, offset, 0xD5800000);
            Set(code, offset + 4, 0x03FE0100);
        }
        constexpr std::array<std::uint32_t, 7> vertex{0x7E020280, 0x7E0402FF, 0x3F800000, 0xF80008CF, 0x02010100, 0xBF810000, 0xBF9F0000};
        constexpr std::array<std::uint32_t, 7> fragment{0x7E0002FF, 0x3C003C00, 0x7E0202FF, 0x3C003C00, 0xF8001C0F, 0x00000100, 0xBF810000};
        auto instructions = pixel ? std::span(fragment) : std::span(vertex);
        for (std::size_t index = 0; index < instructions.size(); ++index) Set(code, 36 + index * 4, instructions[index]);
        for (std::size_t offset = 64; offset < code.size(); offset += 4) Set(code, offset, UINT32_MAX);
        Set(metadata, 0, 2);
        Set(metadata, 4, 36);
        Set(metadata, 8, 16);
        Set(metadata, 12, 0x440);
        Set(metadata, 16, 0x43);
        Set(graphics.state, 56, 0x002C0000);
        Set(graphics.state, 60, 8);
        Set(graphics.state, 76, 0x002C0000);
        Set(graphics.state, 80, 8);
        graphics.Context(4, 2);
        graphics.Context(24, 4);
        Set(graphics.state, 372, 1);
    }

    AnyBox::Shader::SerializedGraphicsStageSource Source(std::uint32_t wave) {
        Fixtures::Set(graphics.state, 312, wave == 32 ? 0x800000 : 0);
        Fixtures::Set(graphics.state, 92, 2 | (wave == 32 ? 0x8000u : 0u));
        return {graphics.state, code, metadata, wave};
    }
};
}
