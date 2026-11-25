#pragma once

#include <xg_xs.hpp>
#include <anybox/layout/Texture.hpp>
#include <atomic>

namespace AnyBox::XG {
class TextureComputer : public IXGTextureComputer {
public:
    explicit TextureComputer(const XGTextureDescription& description);
    static std::uint32_t AddRef(IXGTextureComputer* self);
    static std::uint32_t Release(IXGTextureComputer* self);
    static std::int32_t GetResourceLayout(IXGTextureComputer* self, XGTextureLayout* layout);
    static std::uint64_t GetSize(IXGTextureComputer* self);
    static std::uint64_t GetAlignment(IXGTextureComputer* self);
    static std::uint64_t GetMipOffset(IXGTextureComputer* self, std::uint32_t plane, std::uint32_t mip);
    static std::uint64_t GetTexelOffset(IXGTextureComputer* self, std::uint32_t plane, std::uint32_t mip, std::uint64_t x, std::uint32_t y, std::uint32_t z, std::uint32_t sample, std::uint32_t* bitPosition);
    static std::int32_t GetTexelCoordinates(IXGTextureComputer* self, std::uint64_t offset, std::uint32_t* plane, std::uint32_t* mip, std::uint64_t* x, std::uint32_t* y, std::uint32_t* z, std::uint32_t* sample);
    static std::int32_t CopyIntoSubresource(IXGTextureComputer* self, void* tiled, std::uint32_t plane, std::uint32_t subresource, const void* linear, std::uint32_t rowPitch, std::uint32_t slicePitch);
    static std::int32_t CopyFromSubresource(IXGTextureComputer* self, void* linear, std::uint32_t plane, std::uint32_t subresource, std::uint32_t sample, const void* tiled, std::uint32_t rowPitch, std::uint32_t slicePitch);
    static std::int32_t GetResourceTiling(IXGTextureComputer* self, std::uint32_t* tileCount, XGPackedMipDescription* packedMips, XGTileShape* tileShape, std::uint32_t* tilingCount, std::uint32_t firstSubresource, XGSubresourceTiling* tilings);
    static std::int32_t GetPlaneDescriptor(IXGTextureComputer* self, std::uint32_t plane, std::uint32_t* descriptor);
    static bool IsTiledResource(IXGTextureComputer* self);

private:
    static TextureComputer& from(IXGTextureComputer* self);
    static const XGTextureComputerTable table;
    std::atomic<std::uint32_t> references{1};
    XGTextureDescription description;
    Address::SurfaceLayout surface;
    Address::SurfaceDescription addressing;
    XGTextureLayout layout{};
};
}
