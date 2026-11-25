#pragma once

#include <anybox/shader/SerializedComputeSource.hpp>

namespace AnyBox::Graphics {
struct SerializedComputeDescription {
    void* root;
    const std::byte* dispatchState;
    std::size_t dispatchSize;
    const std::byte* auxiliaryState;
    std::size_t auxiliarySize;
    const std::byte* code;
    std::size_t codeSize;
    const std::byte* metadata;
    std::size_t metadataSize;
};

static_assert(sizeof(SerializedComputeDescription) == 72);
static_assert(offsetof(SerializedComputeDescription, dispatchState) == 8);
static_assert(offsetof(SerializedComputeDescription, auxiliaryState) == 24);
static_assert(offsetof(SerializedComputeDescription, code) == 40);
static_assert(offsetof(SerializedComputeDescription, metadata) == 56);
Shader::SerializedComputeSource ReadSerializedComputeSource(const SerializedComputeDescription& description);
}
