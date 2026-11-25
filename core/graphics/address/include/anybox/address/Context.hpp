#pragma once

#include "Hardware.hpp"
#include <cstdint>
#include <span>

namespace AnyBox::Address {
struct SurfaceDescription {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t slices = 0;
    std::uint32_t mipLevels = 0;
    std::uint32_t samples = 0;
    std::uint32_t fragments = 0;
    std::uint32_t bitsPerElement = 0;
    std::uint32_t format = 0;
    std::uint32_t swizzleMode = 0;
    std::uint32_t resourceType = 0;
    std::uint32_t flags = 0;
    std::uint32_t pitch = 0;
    std::uint32_t sliceAlignment = 0;
};

struct MipLayout {
    std::uint32_t pitch;
    std::uint32_t height;
    std::uint32_t depth;
    std::uint32_t pixelPitch;
    std::uint32_t pixelHeight;
    std::uint32_t equation;
    std::uint64_t offset;
    std::uint64_t blockOffset;
    std::uint32_t tailOffset;
    std::uint32_t tailX;
    std::uint32_t tailY;
    std::uint32_t tailZ;
};

struct SurfaceLayout {
    std::uint64_t size;
    std::uint64_t sliceSize;
    std::uint32_t alignment;
    std::uint32_t pitch;
    std::uint32_t height;
    std::uint32_t slices;
    std::uint32_t pixelPitch;
    std::uint32_t pixelHeight;
    std::uint32_t bitsPerElement;
    std::uint32_t blockWidth;
    std::uint32_t blockHeight;
    std::uint32_t blockSlices;
    std::uint32_t firstMipInTail;
    bool mipChainInTail;
};

struct ElementCoordinate {
    std::uint32_t x = 0;
    std::uint32_t y = 0;
    std::uint32_t slice = 0;
    std::uint32_t mip = 0;
    std::uint32_t sample = 0;
};
struct ElementOffset { std::uint64_t address; std::uint32_t bitPosition; };
struct SurfaceCopy {
    std::uint32_t mip;
    std::uint32_t slice;
    std::uint64_t rowPitch;
    std::uint64_t slicePitch;
};

class Context {
public:
    explicit Context(const HardwareConfiguration& hardware);
    ~Context();
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    ElementOffset GetOffset(const SurfaceDescription& description, const ElementCoordinate& coordinate) const;
    void CopyToSurface(const SurfaceDescription& description, void* surface, const void* memory, const SurfaceCopy& copy) const;
    void CopyFromSurface(const SurfaceDescription& description, void* memory, const void* surface, const SurfaceCopy& copy) const;
    SurfaceLayout Compute(const SurfaceDescription& description, std::span<MipLayout> mips = {}) const;

private:
    void copySurface(const SurfaceDescription& description, void* surface, void* memory, const SurfaceCopy& copy, bool toSurface) const;
    void* handle = nullptr;
    bool surfaceCopy;
};
}
