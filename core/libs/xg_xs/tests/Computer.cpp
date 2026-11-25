#include <xg_xs.hpp>
#include <anybox/ExceptionTest.hpp>
#include <stdexcept>

namespace {
void Require(bool value) { if (!value) throw std::runtime_error("Texture computer layout test failed"); }

template<class TCallable>
void Reject(TCallable&& callable) {
    try { callable(); } catch (const std::exception&) { return; }
    throw std::runtime_error("Texture computer accepted an unsupported operation");
}
}

int main() {
    return AnyBox::Tests::Run([] {
        XGTextureDescription description{};
        description.dimension = 3;
        description.width = 256;
        description.height = 128;
        description.depthOrArraySize = 1;
        description.mipLevels = 1;
        description.format = 61;
        description.sample.count = 1;
        description.layout = 0x105;
        description.flags = 0x300000;
        IXGTextureComputer* computer = nullptr;
        Require(XGCreateTextureComputer(&description, &computer) == 0 && computer);
        const auto& table = *computer->vtable;
        Require(table.AddRef(computer) == 2 && table.Release(computer) == 1);
        XGTextureLayout layout{};
        Require(table.GetResourceLayout(computer, &layout) == 0);
        Require(layout.size == 32768 && layout.alignment == 4096 && layout.mipLevels == 1 && layout.planeCount == 1 && layout.dimension == 3);
        const auto& plane = layout.planes[0];
        Require(plane.usage == 1 && plane.size == layout.size && plane.sliceSize == layout.size && plane.offset == 0 && plane.alignment == layout.alignment && plane.elementBytes == 1 && plane.standardMips == 1 && plane.tailMips == 0);
        const auto& mip = plane.mips[0];
        Require(mip.size == 32768 && mip.offset == 0 && mip.sliceSize == 32768 && mip.pitchPixels == 256 && mip.pitchBytes == 256 && mip.alignment == 4096);
        Require(mip.padded == std::array<std::uint32_t, 3>{256, 128, 1} && mip.elements == std::array<std::uint32_t, 2>{256, 128} && mip.texels == mip.elements);
        Require(mip.samples == 1 && mip.swizzleMode == 5 && mip.depthOrArraySize == 1 && mip.sliceDepth == 1 && mip.inTail == 0 && mip.tailPosition == 0);
        Require(table.GetSize(computer) == layout.size && table.GetAlignment(computer) == layout.alignment && table.GetMipOffset(computer, 0, 0) == 0 && !table.IsTiledResource(computer));
        Require(layout.planes[1].size == 0 && plane.mips[1].size == 0);
        Reject([&] { table.GetResourceLayout(computer, nullptr); });
        Reject([&] { table.GetMipOffset(computer, 1, 0); });
        Reject([&] { table.GetMipOffset(computer, 0, 1); });
        Require(table.GetTexelOffset(computer, 0, 0, 0, 0, 0, 0, nullptr) == 0);
        Require(table.GetTexelOffset(computer, 0, 0, 64, 0, 0, 0, nullptr) == 4096);
        Require(table.GetTexelOffset(computer, 0, 0, 0, 64, 0, 0, nullptr) == 16384);
        Reject([&] { table.GetTexelOffset(computer, 0, 0, 256, 0, 0, 0, nullptr); });
        Require(table.Release(computer) == 0);
        for (auto mode : {5u, 21u, 27u}) {
            description.width = description.height = 1024;
            description.format = 28;
            description.depthOrArraySize = 4;
            description.layout = 0x100 | mode;
            Require(XGCreateTextureComputer(&description, &computer) == 0 && computer);
            Require(computer->vtable->GetResourceLayout(computer, &layout) == 0);
            Require(layout.size == 4 * layout.planes[0].sliceSize && layout.planes[0].mips[0].size == layout.planes[0].sliceSize && layout.planes[0].mips[0].depthOrArraySize == 4);
            Require(computer->vtable->Release(computer) == 0);
        }
        description.mipLevels = 2;
        computer = reinterpret_cast<IXGTextureComputer*>(1);
        Reject([&] { XGCreateTextureComputer(&description, &computer); });
        Require(!computer);
        Reject([&] { XGCreateTextureComputer(&description, nullptr); });
    });
}
