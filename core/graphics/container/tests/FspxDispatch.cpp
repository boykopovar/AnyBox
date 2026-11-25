#include "Fixtures.hpp"
#include <anybox/graphics/container/FspxDispatch.hpp>
#include <array>

void CheckFspxDispatch() {
    using namespace AnyBox::Graphics::Container;
    using namespace Fixtures;
    std::array<std::byte, 28> data{};
    Set(data, 0, 0x4005CF00);
    Set(data, 4, 0x6C090102);
    Set(data, 12, 4 | (8 << 16));
    Set(data, 16, 2);
    Set(data, 20, 0x402C0343);
    Set(data, 24, 8 | 0x380 | 0x1000 | (3 << 15));
    auto selected = ReadSerializedComputeState(data);
    Require(selected.waveSize == 64 && selected.shader.threads == std::array<std::uint32_t, 3>{4, 8, 2});
    Require(selected.shader.declaredSharedBytes == 1536 && selected.shader.resourceControl == (8 | 0x380 | 0x1000 | (3 << 15)));
    Require(selected.shader.floatControl == 0xC0 && selected.shader.dx10Clamp && !selected.shader.ieeeMode);
    Set(data, 16, 0x100002);
    Require(ReadSerializedComputeState(data).waveSize == 32);
    for (std::uint32_t mode = 0; mode < 256; ++mode) {
        for (std::uint32_t flags = 0; flags < 4; ++flags) {
            Set(data, 20, 0x40000343 | (mode << 12) | ((flags & 1) << 21) | ((flags & 2) << 22));
            const auto shader = ReadSerializedComputeState(data).shader;
            Require(shader.floatControl == mode && shader.dx10Clamp == ((flags & 1) != 0) && shader.ieeeMode == ((flags & 2) != 0));
        }
    }
    auto valid = data;
    for (std::size_t size = 0; size < valid.size(); ++size) Reject<std::invalid_argument>([&] { (void)ReadSerializedComputeState(std::span(valid).first(size)); });
    for (const auto& [offset, value] : std::array<std::pair<std::size_t, std::uint32_t>, 8>{{{12, 0}, {12, 1025 | (1 << 16)}, {12, 33 | (32 << 16)}, {16, 0}, {16, 1025}, {16, 33}, {24, 34}, {24, 0x1800}}}) {
        auto invalid = valid;
        Set(invalid, offset, value);
        Reject<std::invalid_argument>([&] { (void)ReadSerializedComputeState(invalid); });
    }
    for (const auto& [offset, value] : std::array<std::pair<std::size_t, std::uint32_t>, 3>{{{0, 0}, {4, 0}, {8, 1}}}) {
        auto invalid = valid;
        Set(invalid, offset, value);
        Reject<std::runtime_error>([&] { (void)ReadSerializedComputeState(invalid); });
    }
    for (auto bit : {10u, 11u, 20u, 22u, 24u, 25u, 26u, 27u, 28u, 29u, 31u}) {
        auto invalid = valid;
        Set(invalid, 20, 1u << bit);
        Reject<std::runtime_error>([&] { (void)ReadSerializedComputeState(invalid); });
    }
    for (auto bit : {0u, 6u, 13u, 14u, 24u, 31u}) {
        auto invalid = valid;
        Set(invalid, 24, 1u << bit);
        Reject<std::runtime_error>([&] { (void)ReadSerializedComputeState(invalid); });
    }
    for (auto bit : {16u, 17u, 18u, 19u, 21u, 31u}) {
        auto invalid = valid;
        Set(invalid, 16, 1u << bit);
        Reject<std::runtime_error>([&] { (void)ReadSerializedComputeState(invalid); });
    }
}
