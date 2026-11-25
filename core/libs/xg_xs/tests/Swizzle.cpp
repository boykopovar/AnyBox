#include <xg_xs.hpp>
#include <anybox/layout/Texture.hpp>
#include <anybox/ExceptionTest.hpp>
#include <stdexcept>

namespace {
void Require(bool value) { if (!value) throw std::runtime_error("Texture swizzle suggestion test failed"); }

template<class TCallable>
void Reject(TCallable&& callable) {
    try { callable(); } catch (const std::exception&) { return; }
    throw std::runtime_error("Unsupported texture swizzle request was accepted");
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
        description.flags = 0x300000;
        std::uint32_t mode = UINT32_MAX;
        Require(XGSuggestSwizzleMode(&description, 0, &mode) == 0 && mode == 0x105);
        Require(XGSuggestSwizzleMode(&description, 0, &description.layout) == 0 && description.layout == mode);
        auto surface = AnyBox::Layout::ComputeTextureSurface(description);
        Require(surface.size == 32768 && surface.alignment == 4096 && surface.pixelPitch == 256 && surface.pixelHeight == 128);
        for (auto format : {2u, 10u, 28u, 61u, 189u, 190u, 191u}) for (auto extent : {1u, 63u, 256u, 1024u}) {
            auto ordinary = description;
            ordinary.width = ordinary.height = extent;
            ordinary.format = format;
            ordinary.layout = 0;
            Require(XGSuggestSwizzleMode(&ordinary, 0, &mode) == 0 && mode == 0x105);
            ordinary.layout = mode;
            Require(AnyBox::Layout::ComputeTextureSurface(ordinary).size > 0);
        }
        description.flags |= 1;
        Require(XGSuggestSwizzleMode(&description, 0, &mode) == 0 && mode == 0x11B);
        description.flags = 4;
        Require(XGSuggestSwizzleMode(&description, 0, &mode) == 0 && mode == 0x115);
        description.width = description.height = 1024;
        Require(XGSuggestSwizzleMode(&description, 0, &mode) == 0 && mode == 0x11B);
        mode = 0x12345678;
        Reject([&] { XGSuggestSwizzleMode(nullptr, 0, &mode); });
        Reject([&] { XGSuggestSwizzleMode(&description, 0, nullptr); });
        Reject([&] { XGSuggestSwizzleMode(&description, 1, &mode); });
        auto invalid = description;
        invalid.width = 0;
        Reject([&] { XGSuggestSwizzleMode(&invalid, 0, &mode); });
        invalid = description;
        invalid.sample.count = 2;
        Reject([&] { XGSuggestSwizzleMode(&invalid, 0, &mode); });
        invalid = description;
        invalid.mipLevels = 0;
        Reject([&] { XGSuggestSwizzleMode(&invalid, 0, &mode); });
        invalid = description;
        invalid.dimension = 4;
        Reject([&] { XGSuggestSwizzleMode(&invalid, 0, &mode); });
        invalid = description;
        invalid.flags |= 0x100;
        Reject([&] { XGSuggestSwizzleMode(&invalid, 0, &mode); });
        Require(mode == 0x12345678);
    });
}
