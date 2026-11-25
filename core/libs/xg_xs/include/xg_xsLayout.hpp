#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

struct XGMipLayout {
    std::uint64_t size;
    std::uint64_t offset;
    std::uint64_t sliceSize;
    std::uint32_t pitchPixels;
    std::uint32_t pitchBytes;
    std::uint32_t alignment;
    std::array<std::uint32_t, 3> padded;
    std::array<std::uint32_t, 2> elements;
    std::array<std::uint32_t, 2> texels;
    std::uint32_t depthOrArraySize;
    std::uint32_t samples;
    std::uint32_t swizzleMode;
    std::array<std::uint64_t, 2> bankRotation;
    std::uint32_t sliceDepth;
    std::uint32_t inTail;
    std::uint32_t tailPosition;
};

struct XGPlaneLayout {
    std::uint32_t usage;
    std::uint64_t size;
    std::uint64_t sliceSize;
    std::uint64_t offset;
    std::uint64_t alignment;
    std::uint32_t elementBytes;
    std::uint32_t standardMips;
    std::uint32_t tailMips;
    std::array<XGMipLayout, 15> mips;
};

struct XGTextureLayout {
    std::uint64_t size;
    std::uint64_t alignment;
    std::uint32_t mipLevels;
    std::uint32_t planeCount;
    std::array<XGPlaneLayout, 4> planes;
    std::uint32_t dimension;
};

struct XGPackedMipDescription {
    std::uint8_t standardMipCount;
    std::uint8_t packedMipCount;
    std::uint32_t packedTileCount;
    std::uint32_t firstTile;
};
struct XGTileShape { std::uint32_t width; std::uint32_t height; std::uint32_t depth; };
struct XGSubresourceTiling { std::uint32_t width; std::uint16_t height; std::uint16_t depth; std::uint32_t firstTile; };
static_assert(sizeof(XGMipLayout) == 0x70 && offsetof(XGMipLayout, pitchBytes) == 0x1C);
static_assert(sizeof(XGPlaneLayout) == 0x6C8 && offsetof(XGPlaneLayout, mips) == 0x38);
static_assert(sizeof(XGTextureLayout) == 0x1B40 && offsetof(XGTextureLayout, planes) == 0x18);
static_assert(sizeof(XGPackedMipDescription) == 12 && sizeof(XGSubresourceTiling) == 12);
