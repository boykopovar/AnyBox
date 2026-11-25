#include "Fixtures.hpp"
#include <anybox/graphics/container/Fspx.hpp>
#include <algorithm>
#include <array>

namespace {
using namespace AnyBox::Graphics::Container;
using namespace Fixtures;

void CheckFspxRoundTrip() {
    std::array<std::byte, 28> state{};
    std::array<std::byte, 3> auxiliary{std::byte{7}, std::byte{8}, std::byte{9}};
    std::array<std::byte, 8> code{std::byte{0}, std::byte{0}, std::byte{0x81}, std::byte{0xBF}};
    std::array<std::byte, 12> records{};
    Set(state, 12, 0x00040008);
    const std::array<Part, 4> parts{{{7, records}, {6, code}, {1, auxiliary}, {0, state}}};
    auto binary = BuildFspx(parts);
    FspxView view(binary);
    Require(binary.size() == 132);
    Require(std::ranges::equal(view.GetDispatchState(), state));
    Require(std::ranges::equal(view.GetAuxiliaryState(), auxiliary));
    Require(std::ranges::equal(view.GetCode(), code));
    Require(std::ranges::equal(view.GetRecords(), records));
    Require(view.GetDispatchState().data() == binary.data() + 80);
    Require(view.GetCode().data() == binary.data() + 112);
    const std::array<Part, 4> reordered{{parts[3], parts[1], parts[0], parts[2]}};
    Require(BuildFspx(reordered) == binary);
    const std::array<Part, 2> required{{{6, code}, {0, state}}};
    auto minimal = BuildFspx(required);
    FspxView minimalView(minimal);
    Require(minimalView.GetAuxiliaryState().empty() && minimalView.GetRecords().empty());
    Require(std::ranges::equal(minimalView.GetCode(), code));
    for (std::size_t size = 0; size < binary.size(); ++size) Reject<std::invalid_argument>([&] { (void)FspxView(std::span(binary).first(size)); });
}

void CheckFspxInvalidParts() {
    const std::array<std::byte, 8> bytes{};
    const std::array<Part, 2> parts{{{0, bytes}, {6, bytes}}};
    auto binary = BuildFspx(parts);
    for (const auto& [offset, value] : std::array<std::pair<std::size_t, std::uint32_t>, 11>{{{0, 0}, {8, UINT32_MAX}, {12, 0}, {20, 16}, {20, UINT32_MAX}, {20, 49}, {24, UINT32_MAX}, {24, 0}, {32, 0}, {36, 48}, {40, 7}}}) {
        auto invalid = binary;
        Set(invalid, offset, value);
        Reject<std::invalid_argument>([&] { (void)FspxView(invalid); });
    }
    for (const auto& [offset, value] : std::array<std::pair<std::size_t, std::uint32_t>, 3>{{{4, 2}, {16, 3}, {28, 1}}}) {
        auto invalid = binary;
        Set(invalid, offset, value);
        Reject<std::runtime_error>([&] { (void)FspxView(invalid); });
    }
    auto missing = binary;
    Set(missing, 8, 1);
    Reject<std::invalid_argument>([&] { (void)FspxView(missing); });
    auto duplicate = parts;
    duplicate[1].tag = 0;
    Reject<std::invalid_argument>([&] { (void)BuildFspx(duplicate); });
    duplicate[1].tag = 3;
    Reject<std::runtime_error>([&] { (void)BuildFspx(duplicate); });
    Reject<std::invalid_argument>([&] { (void)BuildFspx({}); });
    Reject<std::invalid_argument>([&] { (void)BuildFspx(std::span(parts).first(1)); });
    auto empty = parts;
    empty[0].data = {};
    Reject<std::invalid_argument>([&] { (void)BuildFspx(empty); });
    empty = parts;
    empty[1].data = std::span(bytes).first(3);
    Reject<std::invalid_argument>([&] { (void)BuildFspx(empty); });
}
}

void CheckFspx() {
    CheckFspxRoundTrip();
    CheckFspxInvalidParts();
}
