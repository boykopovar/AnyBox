#pragma once

#include "NativeFixtures.hpp"
#include <anybox/graphics/container/View.hpp>

inline std::vector<std::byte> MakeGraphicsSource(bool pixel) {
    auto data = Fixtures::MakeComputeProgram();
    data.resize(0x380);
    Fixtures::Set(data, 0x10, 5);
    Fixtures::Set(data, 0x100, pixel ? 5 : 4);
    Fixtures::Set(data, 0xC0, 0x300);
    Fixtures::Set(data, 0xC8, 0x80);
    for (std::size_t offset = 0x138; offset < 0x300; offset += 4) Fixtures::Set(data, offset, 0);
    for (std::size_t offset = 0x328; offset < data.size(); offset += 4) Fixtures::Set(data, offset, 0xBF9F0000);
    constexpr std::array<std::uint32_t, 7> vertex{0x7E020280, 0x7E0402FF, 0x3F800000, 0xF80008CF, 0x02010100, 0xBF810000, 0xBF9F0000};
    constexpr std::array<std::uint32_t, 7> fragment{0x7E0002FF, 0x3C003C00, 0x7E0202FF, 0x3C003C00, 0xF8001C0F, 0x00000100, 0xBF810000};
    auto instructions = pixel ? std::span(fragment) : std::span(vertex);
    for (std::size_t index = 0; index < instructions.size(); ++index) Fixtures::Set(data, 0x328 + index * 4, instructions[index]);
    auto packed = Fixtures::PackNativeProgram(data);
    auto outer = Fixtures::MakeNativeContainer(packed, {});
    AnyBox::Graphics::Container::DxbcView container(outer);
    auto part = container.FindPart(0x4C495844);
    std::vector<std::byte> program(part->data.begin(), part->data.end());
    Fixtures::Set(program, 0, pixel ? 0x66 : 0x10066);
    Fixtures::Set(program, 28, 2);
    return AnyBox::Graphics::Container::BuildDxbc(std::array<AnyBox::Graphics::Container::Part, 1>{{{0x4C495844, program}}});
}
