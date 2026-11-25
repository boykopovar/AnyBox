#pragma once

#include "NativeFixtures.hpp"
#include <anybox/graphics/container/Fspx.hpp>
#include <algorithm>
#include <array>

namespace SerializedTests {
struct ReferenceFixture {
    std::vector<std::byte> program = Fixtures::MakeComputeProgram();
    std::array<std::byte, 28> dispatch{};
    std::array<std::byte, 40> code{};
    std::array<std::byte, 56> records{};
    std::array<std::byte, 20> hash{};
    std::vector<std::byte> root;

    ReferenceFixture() {
        using Fixtures::Set;
        Set(dispatch, 0, 0x4005CF00);
        Set(dispatch, 4, 0x6C090102);
        Set(dispatch, 12, 4 | (2 << 16));
        Set(dispatch, 16, 0x100002);
        Set(dispatch, 20, 0x402C0000);
        Set(dispatch, 24, (2 << 15) | (2 << 11) | 0x380 | 8);
        const std::array<std::uint32_t, 10> words{0xD5801234, 0x03FF13AB, 0x10203040, 0xD5805678, 0x03FFFDDC, 0x55667788, 0xD5800000, 0x03FE0100, 7, 0xBF810000};
        for (std::size_t index = 0; index < words.size(); ++index) Set(code, index * 4, words[index]);
        Set(hash, 4, 0x89AB1234);
        Set(hash, 8, 0x10203040);
        Set(hash, 12, 0xFEDC5678);
        Set(hash, 16, 0x55667788);
        Set(records, 0, 0x403);
        Set(records, 4, 36);
        Set(records, 8, 10);
        Set(records, 20, 0x440);
        Set(records, 24, 0x43);
        std::ranges::copy(std::span(hash).subspan(4), records.begin() + 28);
        std::array<std::byte, 24> signature{};
        Set(signature, 0, 2);
        Set(signature, 8, 24);
        Set(signature, 16, 24);
        root = AnyBox::Graphics::Container::BuildDxbc(std::array<AnyBox::Graphics::Container::Part, 1>{{{0x30535452, signature}}});
    }

    std::vector<std::byte> Source() const {
        using namespace AnyBox::Graphics::Container;
        auto compressed = Fixtures::PackNativeProgram(program);
        auto native = Fixtures::MakeNativeContainer(compressed, compressed);
        DxbcView view(native);
        auto bytes = view.GetPart(0).data;
        std::vector<std::byte> dxil(bytes.begin(), bytes.end());
        Fixtures::Set(dxil, 28, 2);
        return BuildDxbc(std::array<Part, 2>{{{0x4C495844, dxil}, {0x48534148, hash}}});
    }

    std::vector<std::byte> Payload() const {
        using namespace AnyBox::Graphics::Container;
        return BuildFspx(std::array<Part, 3>{{{0, dispatch}, {6, code}, {7, records}}});
    }
};
}
