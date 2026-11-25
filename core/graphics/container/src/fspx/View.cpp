#include <anybox/graphics/container/Fspx.hpp>
#include "Parts.hpp"
#include "../Integer.hpp"

AnyBox::Graphics::Container::FspxView::FspxView(std::span<const std::byte> data) {
    if (data.size() < 16 || ReadInteger(data, 0) != 0x58505346) throw std::invalid_argument("Xbox serialized pipeline header is invalid");
    if (ReadInteger(data, 4) != 1) throw std::runtime_error("Xbox serialized pipeline version is not implemented");
    if (data.size() != ReadInteger(data, 12)) throw std::invalid_argument("Xbox serialized pipeline size is invalid");
    auto count = ReadInteger(data, 8);
    if (count > (data.size() - 16) / 16) throw std::invalid_argument("Xbox serialized pipeline directory is truncated");
    auto directoryEnd = 16 + static_cast<std::size_t>(count) * 16;
    std::array<bool, 4> found{};
    std::array<std::size_t, 4> offsets{};

    for (std::uint32_t entry = 0; entry < count; ++entry) {
        auto item = 16 + static_cast<std::size_t>(entry) * 16;
        auto index = FspxPartIndex(ReadInteger(data, item));
        if (found[index]) throw std::invalid_argument("Xbox serialized pipeline contains duplicate parts");
        if (ReadInteger(data, item + 12) != 0) throw std::runtime_error("Xbox serialized pipeline part flags are not implemented");
        auto offset = ReadInteger(data, item + 4);
        auto size = ReadInteger(data, item + 8);
        if (offset % 4 != 0 || offset < directoryEnd || offset > data.size() || size > data.size() - offset) throw std::invalid_argument("Xbox serialized pipeline part range is invalid");
        for (std::size_t previous = 0; previous < parts.size(); ++previous) {
            if (found[previous] && size != 0 && !parts[previous].empty() && offset < offsets[previous] + parts[previous].size() && offsets[previous] < static_cast<std::size_t>(offset) + size) throw std::invalid_argument("Xbox serialized pipeline parts overlap");
        }
        found[index] = true;
        offsets[index] = offset;
        parts[index] = data.subspan(offset, size);
    }

    if (parts[0].empty() || parts[2].empty()) throw std::invalid_argument("Xbox serialized pipeline requires dispatch state and code");
    if (parts[2].size() % 4 != 0) throw std::invalid_argument("Xbox serialized pipeline code is not word aligned");
}
