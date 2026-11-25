#include <xg_xs.hpp>
#include <anybox/ExceptionTest.hpp>
#include <vector>
#include <iostream>
#include <stdexcept>

namespace {
void Require(bool value) { if (!value) throw std::runtime_error("Texture computer surface copy test failed"); }

template<class TCallable>
void Reject(TCallable&& callable) {
    try { callable(); } catch (const std::exception&) { return; }
    throw std::runtime_error("Invalid texture surface copy was accepted");
}

void Check(std::uint32_t format, std::uint32_t bytes, std::uint32_t mode) {
    std::cerr << "texture_copy format=" << format << " mode=" << mode << std::endl;
    XGTextureDescription description{};
    description.dimension = 3;
    description.width = 257;
    description.height = 129;
    description.depthOrArraySize = 3;
    description.mipLevels = 1;
    description.format = format;
    description.sample.count = 1;
    description.layout = 0x100 | mode;
    description.flags = 0x300000;
    IXGTextureComputer* computer = nullptr;
    Require(XGCreateTextureComputer(&description, &computer) == 0 && computer);
    const auto& table = *computer->vtable;
    XGTextureLayout layout{};
    Require(table.GetResourceLayout(computer, &layout) == 0);
    std::vector<std::byte> tiled(static_cast<std::size_t>(layout.size) + 32, std::byte{0xA5});
    auto row = static_cast<std::uint32_t>(description.width) * bytes + 13;
    auto destinationRow = row + 7;
    std::vector<std::vector<std::byte>> sources(3, std::vector<std::byte>(row * description.height, std::byte{0xCC}));
    for (std::uint32_t slice = 0; slice < 3; ++slice) {
        auto& source = sources[slice];
        for (std::uint32_t y = 0; y < description.height; ++y) for (std::uint32_t x = 0; x < description.width * bytes; ++x) source[y * row + x] = static_cast<std::byte>((y * 17 + x * 13 + slice * 31) & 255);
        Require(table.CopyIntoSubresource(computer, tiled.data(), 0, slice, source.data(), row, 0) == 0);
    }
    for (std::uint32_t slice = 0; slice < 3; ++slice) {
        std::vector<std::byte> destination(destinationRow * description.height, std::byte{0xDD});
        Require(table.CopyFromSubresource(computer, destination.data(), 0, slice, 0, tiled.data(), destinationRow, 0) == 0);
        for (std::uint32_t y = 0; y < description.height; ++y) {
            for (std::uint32_t x = 0; x < description.width * bytes; ++x) Require(destination[y * destinationRow + x] == sources[slice][y * row + x]);
            for (std::uint32_t x = static_cast<std::uint32_t>(description.width) * bytes; x < destinationRow; ++x) Require(destination[y * destinationRow + x] == std::byte{0xDD});
        }
        for (auto x : {0u, 1u, 15u, 63u, 128u, 256u}) for (auto y : {0u, 1u, 17u, 64u, 128u}) {
            std::uint32_t bit = UINT32_MAX;
            auto offset = table.GetTexelOffset(computer, 0, 0, x, y, slice, 0, &bit);
            Require(bit == 0 && offset + bytes <= layout.size);
            for (std::uint32_t index = 0; index < bytes; ++index) Require(tiled[static_cast<std::size_t>(offset) + index] == sources[slice][y * row + x * bytes + index]);
        }
    }
    for (std::size_t index = static_cast<std::size_t>(layout.size); index < tiled.size(); ++index) Require(tiled[index] == std::byte{0xA5});
    Reject([&] { table.CopyIntoSubresource(computer, tiled.data(), 1, 0, sources[0].data(), row, 0); });
    Reject([&] { table.CopyIntoSubresource(computer, tiled.data(), 0, 3, sources[0].data(), row, 0); });
    Reject([&] { table.CopyIntoSubresource(computer, nullptr, 0, 0, sources[0].data(), row, 0); });
    Reject([&] { table.CopyIntoSubresource(computer, tiled.data(), 0, 0, nullptr, row, 0); });
    Reject([&] { table.CopyIntoSubresource(computer, tiled.data(), 0, 0, sources[0].data(), 1, 0); });
    Reject([&] { table.CopyFromSubresource(computer, sources[0].data(), 0, 0, 1, tiled.data(), row, 0); });
    Require(table.Release(computer) == 0);
}
}

int main() {
    return AnyBox::Tests::Run([] {
        for (auto mode : {0u, 5u, 21u, 27u}) {
            Check(61, 1, mode);
            Check(28, 4, mode);
            Check(10, 8, mode);
            Check(2, 16, mode);
        }
    });
}
