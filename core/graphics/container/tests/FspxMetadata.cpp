#include "Fixtures.hpp"
#include <anybox/graphics/container/FspxMetadata.hpp>
#include <algorithm>
#include <array>

namespace {
using namespace Fixtures;
using namespace AnyBox::Graphics::Container;

struct MetadataFixture {
    std::array<std::byte, 28> dispatch{};
    std::array<std::byte, 40> code{};
    std::array<std::byte, 120> records{};
    std::array<std::byte, 16> hash{};

    MetadataFixture() {
        const std::array<std::uint32_t, 10> words{0xD5801234, 0x03FF13AB, 0x10203040, 0xD5805678, 0x03FFFDDC, 0x55667788, 0xD5800000, 0x03FE0100, 7, 0xBF810000};
        for (std::size_t index = 0; index < words.size(); ++index) Set(code, index * 4, words[index]);
        Set(hash, 0, 0x89AB1234);
        Set(hash, 4, 0x10203040);
        Set(hash, 8, 0xFEDC5678);
        Set(hash, 12, 0x55667788);
        Set(records, 0, 0x403);
        Set(records, 4, 100);
        Set(records, 8, 10);
        Set(records, 20, 0x10440);
        Set(records, 24, 0x41);
        Set(records, 28, 0x300);
        Set(records, 36, 3);
        Set(records, 40, 4);
        Set(records, 44, 0x408);
        Set(records, 48, UINT32_MAX);
        Set(records, 56, 0x42);
        Set(records, 80, UINT32_MAX);
        Set(records, 88, 0x43);
        std::ranges::copy(hash, records.begin() + 92);
    }

    std::vector<std::byte> Payload(std::span<const std::byte> metadata) const {
        return BuildFspx(std::array<Part, 3>{{{0, dispatch}, {6, code}, {7, metadata}}});
    }
};
}

void CheckFspxMetadata() {
    MetadataFixture fixture;
    auto binary = fixture.Payload(fixture.records);
    auto metadata = ReadSerializedComputeMetadata(FspxView(binary));
    Require(metadata.shaderHash == fixture.hash && metadata.resources.size() == 1);
    NativeResourceBinding expected{NativeResourceKind::ShaderResource, NativeRootKind::Table, 0, 3, 4, 8, 4, 0};
    Require(metadata.resources.front() == expected);
    for (std::size_t size = 0; size < fixture.records.size(); ++size) {
        auto truncated = fixture.Payload(std::span(fixture.records).first(size));
        Reject<std::exception>([&] { (void)ReadSerializedComputeMetadata(FspxView(truncated)); });
    }
    for (const auto& [offset, value] : std::array<std::pair<std::size_t, std::uint32_t>, 17>{{{0, 0}, {4, 0}, {8, 1}, {12, 1}, {16, 1}, {20, 0x18440}, {20, 0x20440}, {24, 0x44}, {28, 0x30200}, {48, 0}, {52, 1}, {56, 0x41}, {80, 0}, {88, 0x42}, {108, 1}, {92, 0}, {84, 1}}}) {
        auto invalid = fixture.records;
        Set(invalid, offset, value);
        auto payload = fixture.Payload(invalid);
        Reject<std::exception>([&] { (void)ReadSerializedComputeMetadata(FspxView(payload)); });
    }
    for (auto offset : {0u, 4u, 8u, 12u, 16u, 20u, 24u, 28u}) {
        auto invalid = fixture;
        invalid.code[offset] ^= std::byte{1};
        auto payload = invalid.Payload(invalid.records);
        Reject<std::exception>([&] { (void)ReadSerializedComputeMetadata(FspxView(payload)); });
    }
}
