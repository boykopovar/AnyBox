#include <anybox/graphics/container/Dxbc.hpp>
#include <anybox/graphics/container/View.hpp>
#include <anybox/ExceptionTest.hpp>
#include <algorithm>
#include <array>
#include <stdexcept>

void CheckDxil();
void CheckCompilerOptions();
void CheckNative();
void CheckNativeComputeState();
void CheckNativeStageState();
void CheckNativeInterface();
void CheckNativeBindings();
void CheckRootSignatureLayout();
void CheckEmbeddedDxbc();
void CheckFspx();
void CheckFspxDispatch();
void CheckFspxMetadata();
void CheckSerializedStageMetadata();

namespace {
using namespace AnyBox::Graphics::Container;
void Require(bool value) {
    if (!value) throw std::runtime_error("DXBC container test failed");
}

template<class TException, class TCallable>
void Reject(TCallable&& callable) {
    bool caught = false;
    try { callable(); } catch (const TException&) { caught = true; }
    Require(caught);
}

void Set(std::span<std::byte> data, std::size_t offset, std::uint32_t value) {
    for (std::size_t index = 0; index < 4; ++index) data[offset + index] = static_cast<std::byte>(value >> (index * 8));
}

void Rehash(std::span<std::byte> data) {
    auto checksum = ComputeDxbcChecksum(data.subspan(20));
    std::ranges::copy(checksum, data.begin() + 4);
}

void RoundTrip() {
    const std::array<std::byte, 5> first{std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, std::byte{5}};
    const std::array<std::byte, 3> second{std::byte{9}, std::byte{8}, std::byte{7}};
    const std::array<Part, 3> parts{{{0x12345678, first}, {0x87654321, second}, {0x11223344, {}}}};
    auto data = BuildDxbc(parts);
    DxbcView view(data);
    Require(view.GetPartCount() == parts.size() && data.size() == 80);
    for (std::uint32_t index = 0; index < parts.size(); ++index) {
        auto restored = view.GetPart(index);
        Require(restored.tag == parts[index].tag && std::ranges::equal(restored.data, parts[index].data));
        Require(view.FindPart(restored.tag)->data.data() == restored.data.data());
    }
    Require(!view.FindPart(0));
    Reject<std::out_of_range>([&] { view.GetPart(3); });
    Reject<std::out_of_range>([&] { view.GetPart(UINT32_MAX); });
    auto rebuilt = BuildDxbc(std::array{view.GetPart(0), view.GetPart(1), view.GetPart(2)});
    Require(data == rebuilt);
    auto empty = BuildDxbc({});
    Require(empty.size() == 32 && DxbcView(empty).GetPartCount() == 0);
}

void InvalidContainers() {
    const std::array<std::byte, 8> bytes{};
    auto source = BuildDxbc(std::array<Part, 2>{{{1, bytes}, {2, bytes}}});
    for (std::size_t size = 0; size < source.size(); ++size) Reject<std::invalid_argument>([&] { DxbcView view{std::span(source).first(size)}; });
    auto invalid = source;
    invalid.back() ^= std::byte{1};
    Reject<std::invalid_argument>([&] { DxbcView view(invalid); });
    for (const auto& [offset, value] : std::array<std::pair<std::size_t, std::uint32_t>, 7>{{{20, 2}, {24, 0}, {28, UINT32_MAX}, {32, 0}, {32, UINT32_MAX}, {36, 40}, {44, UINT32_MAX}}}) {
        invalid = source;
        Set(invalid, offset, value);
        Rehash(invalid);
        Reject<std::invalid_argument>([&] { DxbcView view(invalid); });
    }
    auto duplicate = BuildDxbc(std::array<Part, 2>{{{1, bytes}, {1, bytes}}});
    DxbcView view(duplicate);
    Reject<std::invalid_argument>([&] { view.FindPart(1); });
}
}

int main() {
    return AnyBox::Tests::Run([] {
        RoundTrip();
        InvalidContainers();
        CheckDxil();
        CheckCompilerOptions();
        CheckNative();
        CheckNativeComputeState();
        CheckNativeStageState();
        CheckNativeInterface();
        CheckNativeBindings();
        CheckRootSignatureLayout();
        CheckEmbeddedDxbc();
        CheckFspx();
        CheckFspxDispatch();
        CheckFspxMetadata();
        CheckSerializedStageMetadata();
    });
}
