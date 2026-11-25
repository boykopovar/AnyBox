#pragma once

#include "xg_xsLayout.hpp"

struct IXGTextureComputer;
struct XGTextureComputerTable {
    std::uint32_t (*AddRef)(IXGTextureComputer* self);
    std::uint32_t (*Release)(IXGTextureComputer* self);
    std::int32_t (*GetResourceLayout)(IXGTextureComputer* self, XGTextureLayout* layout);
    std::uint64_t (*GetSize)(IXGTextureComputer* self);
    std::uint64_t (*GetAlignment)(IXGTextureComputer* self);
    std::uint64_t (*GetMipOffset)(IXGTextureComputer* self, std::uint32_t plane, std::uint32_t mip);
    std::uint64_t (*GetTexelOffset)(IXGTextureComputer* self, std::uint32_t plane, std::uint32_t mip, std::uint64_t x, std::uint32_t y, std::uint32_t z, std::uint32_t sample, std::uint32_t* bitPosition);
    std::int32_t (*GetTexelCoordinates)(IXGTextureComputer* self, std::uint64_t offset, std::uint32_t* plane, std::uint32_t* mip, std::uint64_t* x, std::uint32_t* y, std::uint32_t* z, std::uint32_t* sample);
    std::int32_t (*CopyIntoSubresource)(IXGTextureComputer* self, void* tiled, std::uint32_t plane, std::uint32_t subresource, const void* linear, std::uint32_t rowPitch, std::uint32_t slicePitch);
    std::int32_t (*CopyFromSubresource)(IXGTextureComputer* self, void* linear, std::uint32_t plane, std::uint32_t subresource, std::uint32_t sample, const void* tiled, std::uint32_t rowPitch, std::uint32_t slicePitch);
    std::int32_t (*GetResourceTiling)(IXGTextureComputer* self, std::uint32_t* tileCount, XGPackedMipDescription* packedMips, XGTileShape* tileShape, std::uint32_t* tilingCount, std::uint32_t firstSubresource, XGSubresourceTiling* tilings);
    std::int32_t (*GetPlaneDescriptor)(IXGTextureComputer* self, std::uint32_t plane, std::uint32_t* descriptor);
    bool (*IsTiledResource)(IXGTextureComputer* self);
};

struct IXGTextureComputer { const XGTextureComputerTable* vtable; };
static_assert(sizeof(XGTextureComputerTable) == 13 * sizeof(void (*)()));
