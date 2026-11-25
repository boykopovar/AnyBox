#include "Fixtures.hpp"
#include <anybox/graphics/container/SerializedStage.hpp>
#include <algorithm>
#include <array>

namespace {
using namespace Fixtures;
using namespace AnyBox::Graphics::Container;

struct StageFixture {
    std::array<std::byte, 128> code{};
    std::array<std::byte, 112> records{};
    std::array<std::byte, 16> hash{};

    StageFixture() {
        const std::array<std::uint32_t, 10> words{0xD5801234, 0x03FF13AB, 0x10203040, 0xD5805678, 0x03FFFDDC, 0x55667788, 0xD5800000, 0x03FE0100, 7, 0xBF810000};
        for (std::size_t index = 0; index < words.size(); ++index) Set(code, index * 4, words[index]);
        Set(hash, 0, 0x89AB1234);
        Set(hash, 4, 0x10203040);
        Set(hash, 8, 0xFEDC5678);
        Set(hash, 12, 0x55667788);
        Set(records, 0, 2);
        Set(records, 4, 100);
        Set(records, 8, 30);
        Set(records, 12, 0x10440);
        Set(records, 16, 0x41);
        Set(records, 20, 0x300);
        Set(records, 24, 16);
        Set(records, 28, 13);
        Set(records, 32, 128);
        Set(records, 36, 0x408);
        Set(records, 40, UINT32_MAX);
        Set(records, 48, 0x42);
        Set(records, 56, 0xC0001);
        Set(records, 60, 13);
        Set(records, 64, 1);
        Set(records, 72, UINT32_MAX);
        Set(records, 80, 0x43);
        std::ranges::copy(hash, records.begin() + 84);
    }
};
}

void CheckSerializedStageMetadata() {
    StageFixture fixture;
    auto output = ReadSerializedStageMetadata(fixture.code, fixture.records);
    Require(output.codeWords == 30 && output.control == 0x440 && output.shaderHash == fixture.hash);
    NativeResourceBinding resource{NativeResourceKind::ShaderResource, NativeRootKind::Table, 16, 13, 128, 8, 4, 0};
    Require(output.resources.size() == 1 && output.resources.front() == resource);
    Require(output.declarations.size() == 1 && std::ranges::equal(output.declarations.front(), std::span(fixture.records).subspan(52, 20)));

    auto variant = fixture.records;
    Set(variant, 12, 0x18440);
    Require(ReadSerializedStageMetadata(fixture.code, variant).control == 0x8440);
    Set(variant, 8, 32);
    Require(ReadSerializedStageMetadata(fixture.code, variant).codeWords == 32);
    for (std::size_t size = 0; size < fixture.records.size(); ++size) Reject<std::exception>([&] { (void)ReadSerializedStageMetadata(fixture.code, std::span(fixture.records).first(size)); });
    for (std::size_t size = 0; size < 120; ++size) Reject<std::exception>([&] { (void)ReadSerializedStageMetadata(std::span(fixture.code).first(size), fixture.records); });

    for (const auto& [offset, value] : std::array<std::pair<std::size_t, std::uint32_t>, 15>{{{0, 0}, {4, 0}, {8, 0}, {8, 33}, {8, 8}, {12, 0x20440}, {16, 0x44}, {40, 0}, {44, 1}, {48, 0x41}, {72, 0}, {76, 1}, {80, 0x42}, {84, 0}, {100, 1}}}) {
        auto invalid = fixture.records;
        Set(invalid, offset, value);
        Reject<std::exception>([&] { (void)ReadSerializedStageMetadata(fixture.code, invalid); });
    }
    for (auto offset : {0u, 4u, 8u, 12u, 16u, 20u, 24u, 28u}) {
        auto invalid = fixture.code;
        invalid[offset] ^= std::byte{1};
        Reject<std::exception>([&] { (void)ReadSerializedStageMetadata(invalid, fixture.records); });
    }
    auto misplaced = fixture.records;
    std::swap_ranges(misplaced.begin() + 48, misplaced.begin() + 80, misplaced.begin() + 80);
    Reject<std::exception>([&] { (void)ReadSerializedStageMetadata(fixture.code, misplaced); });
}
