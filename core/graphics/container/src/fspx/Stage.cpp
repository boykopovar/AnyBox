#include <anybox/graphics/container/SerializedStage.hpp>
#include "Hash.hpp"
#include "../Integer.hpp"
#include <algorithm>

AnyBox::Graphics::Container::SerializedStageMetadata AnyBox::Graphics::Container::ReadSerializedStageMetadata(std::span<const std::byte> code, std::span<const std::byte> records) {
    if (code.empty() || code.size() % 4 != 0 || code.size() > UINT32_MAX) throw std::invalid_argument("Xbox serialized stage code size is invalid");
    if (records.size() < 48 || (records.size() - 16) % 32 != 0) throw std::invalid_argument("Xbox serialized stage metadata size is invalid");
    if (ReadInteger(records, 0) != 2) throw std::runtime_error("Xbox serialized stage metadata version is not implemented");
    if (ReadInteger(records, 4) != records.size() - 12) throw std::invalid_argument("Xbox serialized stage metadata size disagrees with its payload");
    auto codeWords = ReadInteger(records, 8);
    if (codeWords == 0 || codeWords > code.size() / 4) throw std::invalid_argument("Xbox serialized stage code range exceeds its payload");
    auto control = ReadInteger(records, 12);
    SerializedStageMetadata output{codeWords, static_cast<std::uint16_t>(control), {}, {}, {}};
    bool declarations = false;
    bool hash = false;

    for (std::size_t offset = 16; offset < records.size(); offset += 32) {
        auto tag = ReadInteger(records, offset);
        if (tag == 0x43) {
            if (offset + 32 != records.size()) throw std::invalid_argument("Xbox serialized stage hash record is not last");
            if (!std::ranges::all_of(records.subspan(offset + 20, 12), [](auto value) { return value == std::byte{}; })) throw std::runtime_error("Xbox serialized stage hash record flags are not implemented");
            std::ranges::copy(records.subspan(offset + 4, 16), output.shaderHash.begin());
            hash = true;
            continue;
        }
        if (tag != 0x41 && tag != 0x42) throw std::runtime_error("Xbox serialized stage metadata record is not implemented");
        if (ReadInteger(records, offset + 24) != UINT32_MAX || ReadInteger(records, offset + 28) != 0) throw std::runtime_error("Xbox serialized stage binding record flags are not implemented");
        auto payload = records.subspan(offset + 4, 20);
        if (tag == 0x42) {
            declarations = true;
            std::array<std::byte, 20> declaration{};
            std::ranges::copy(payload, declaration.begin());
            output.declarations.push_back(declaration);
        } else {
            if (declarations) throw std::invalid_argument("Xbox serialized stage resource records follow declarations");
            output.resources.push_back(ReadNativeResourceBinding(payload));
        }
    }

    if (!hash || output.resources.size() != control >> 16) throw std::invalid_argument("Xbox serialized stage resource count or hash record is invalid");
    if (output.shaderHash != ReadSerializedCodeHash(code.first(static_cast<std::size_t>(codeWords) * 4))) throw std::invalid_argument("Xbox serialized stage code and metadata shader hashes disagree");
    return output;
}
