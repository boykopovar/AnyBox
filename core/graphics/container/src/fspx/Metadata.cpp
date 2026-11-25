#include <anybox/graphics/container/FspxMetadata.hpp>
#include "Hash.hpp"
#include "../Integer.hpp"
#include <algorithm>

AnyBox::Graphics::Container::SerializedComputeMetadata AnyBox::Graphics::Container::ReadSerializedComputeMetadata(const FspxView& program) {
    return ReadSerializedComputeMetadata(program.GetCode(), program.GetRecords());
}

AnyBox::Graphics::Container::SerializedComputeMetadata AnyBox::Graphics::Container::ReadSerializedComputeMetadata(std::span<const std::byte> code, std::span<const std::byte> data) {
    if (code.empty() || code.size() % 4 != 0 || code.size() > UINT32_MAX) throw std::invalid_argument("Xbox serialized compute code size is invalid");
    if (data.size() < 56 || (data.size() - 24) % 32 != 0) throw std::invalid_argument("Xbox serialized compute metadata size is invalid");
    if (ReadInteger(data, 0) != 0x403) throw std::runtime_error("Xbox serialized compute metadata version is not implemented");
    if (ReadInteger(data, 4) != data.size() - 20 || ReadInteger(data, 8) != code.size() / 4) throw std::invalid_argument("Xbox serialized compute metadata sizes disagree with their payloads");
    if (ReadInteger(data, 12) != 0 || ReadInteger(data, 16) != 0 || (ReadInteger(data, 20) & 65535) != 0x440) throw std::runtime_error("Xbox serialized compute metadata control mode is not implemented");
    SerializedComputeMetadata output{};
    bool declarations = false;
    bool hash = false;
    for (std::size_t offset = 24; offset < data.size(); offset += 32) {
        auto tag = ReadInteger(data, offset);
        if (tag == 0x43) {
            if (offset + 32 != data.size()) throw std::invalid_argument("Xbox serialized shader hash record is not last");
            if (!std::ranges::all_of(data.subspan(offset + 20, 12), [](auto value) { return value == std::byte{}; })) throw std::runtime_error("Xbox serialized shader hash record flags are not implemented");
            std::ranges::copy(data.subspan(offset + 4, 16), output.shaderHash.begin());
            hash = true;
            continue;
        }
        if (tag != 0x41 && tag != 0x42) throw std::runtime_error("Xbox serialized compute metadata record is not implemented");
        if (ReadInteger(data, offset + 24) != UINT32_MAX || ReadInteger(data, offset + 28) != 0) throw std::runtime_error("Xbox serialized compute binding record flags are not implemented");
        if (tag == 0x42) declarations = true;
        else {
            if (declarations) throw std::invalid_argument("Xbox serialized compute resource records follow declarations");
            output.resources.push_back(ReadNativeResourceBinding(data.subspan(offset + 4, 20)));
        }
    }
    if (!hash || output.resources.size() != ReadInteger(data, 20) >> 16) throw std::invalid_argument("Xbox serialized compute resource count or hash record is invalid");
    if (output.shaderHash != ReadSerializedCodeHash(code)) throw std::invalid_argument("Xbox serialized compute code and metadata shader hashes disagree");
    return output;
}
