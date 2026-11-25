#include "Description.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace {
std::span<const std::byte> Bytes(const std::byte* address, std::size_t size) {
    if (!address || size == 0 || size > UINT32_MAX) throw std::invalid_argument("Serialized compute pipeline source part is null or has an invalid size");
    return {address, size};
}
}

AnyBox::Shader::SerializedComputeSource AnyBox::Graphics::ReadSerializedComputeSource(const SerializedComputeDescription& description) {
    Shader::SerializedComputeSource source{Bytes(description.dispatchState, description.dispatchSize), {}, Bytes(description.code, description.codeSize), Bytes(description.metadata, description.metadataSize)};
    if (description.auxiliarySize != 0) {
        source.auxiliaryState = Bytes(description.auxiliaryState, description.auxiliarySize);
        std::uint32_t version = 0;
        if (source.auxiliaryState.size() != 20) throw std::runtime_error("Serialized compute pipeline auxiliary state size is not implemented");
        std::memcpy(&version, source.auxiliaryState.data() + 4, 4);
        if (version != 3 || !std::ranges::all_of(source.auxiliaryState.subspan(8), [](auto value) { return value == std::byte{}; })) throw std::runtime_error("Serialized compute pipeline auxiliary state mode is not implemented");
    }
    return source;
}
