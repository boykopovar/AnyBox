#include <anybox/graphics/container/Fspx.hpp>
#include "Parts.hpp"
#include "../Integer.hpp"
#include <algorithm>

std::vector<std::byte> AnyBox::Graphics::Container::BuildFspx(std::span<const Part> parts) {
    std::array<const Part*, 4> ordered{};
    for (const auto& part : parts) {
        auto index = FspxPartIndex(part.tag);
        if (ordered[index]) throw std::invalid_argument("Xbox serialized pipeline contains duplicate parts");
        ordered[index] = &part;
    }
    std::uint64_t size = 16 + parts.size() * 16;
    for (const auto* part : ordered) {
        if (!part) continue;
        size = (size + 3) & ~std::uint64_t{3};
        size += part->data.size();
        if (size > UINT32_MAX) throw std::length_error("Xbox serialized pipeline size overflows");
    }
    std::vector<std::byte> output(static_cast<std::size_t>(size));
    WriteInteger(output, 0, 0x58505346);
    WriteInteger(output, 4, 1);
    WriteInteger(output, 8, static_cast<std::uint32_t>(parts.size()));
    WriteInteger(output, 12, static_cast<std::uint32_t>(size));
    std::size_t item = 16;
    std::size_t offset = 16 + parts.size() * 16;
    for (const auto* part : ordered) {
        if (!part) continue;
        offset = (offset + 3) & ~std::size_t{3};
        WriteInteger(output, item, part->tag);
        WriteInteger(output, item + 4, static_cast<std::uint32_t>(offset));
        WriteInteger(output, item + 8, static_cast<std::uint32_t>(part->data.size()));
        std::ranges::copy(part->data, output.begin() + static_cast<std::ptrdiff_t>(offset));
        offset += part->data.size();
        item += 16;
    }
    (void)FspxView(output);
    return output;
}
