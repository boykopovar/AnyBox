#include <anybox/address/Context.hpp>
#include <addrinterface.h>
#include <algorithm>
#include <stdexcept>
#include <string>

AnyBox::Address::ElementOffset AnyBox::Address::Context::GetOffset(const SurfaceDescription& description, const ElementCoordinate& coordinate) const {
    if (description.swizzleMode != 0 && description.swizzleMode != 5 && description.swizzleMode != 21 && description.swizzleMode != 27) throw std::runtime_error("Surface addressing swizzle mode is not implemented");
    if ((description.flags & ~0x10601u) != 0) throw std::runtime_error("Surface addressing flags are not implemented");
    if (description.format != 0 || description.resourceType != 1 || description.samples != 1 || description.fragments != 1) throw std::runtime_error("Surface address format, dimension or samples are not implemented");
    if (description.width == 0 || description.height == 0 || description.width > 16384 || description.height > 16384 || description.slices > 2048 || description.mipLevels == 0 || description.mipLevels > 16 || coordinate.mip >= description.mipLevels || coordinate.sample >= description.samples || coordinate.slice >= description.slices) throw std::invalid_argument("Surface address dimensions or subresource are invalid");
    if (coordinate.x >= std::max(1u, description.width >> coordinate.mip) || coordinate.y >= std::max(1u, description.height >> coordinate.mip)) throw std::out_of_range("Surface address coordinates exceed its dimensions");
    if (description.bitsPerElement < 8 || description.bitsPerElement > 128 || (description.bitsPerElement & (description.bitsPerElement - 1)) != 0) throw std::runtime_error("Surface address element size is not implemented");
    ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT input{};
    input.size = sizeof(input);
    input.x = coordinate.x;
    input.y = coordinate.y;
    input.slice = coordinate.slice;
    input.sample = coordinate.sample;
    input.mipId = coordinate.mip;
    input.swizzleMode = static_cast<AddrSwizzleMode>(description.swizzleMode);
    input.flags.value = description.flags;
    input.resourceType = static_cast<AddrResourceType>(description.resourceType);
    input.bpp = description.bitsPerElement;
    input.unalignedWidth = description.width;
    input.unalignedHeight = description.height;
    input.numSlices = description.slices;
    input.numMipLevels = description.mipLevels;
    input.numSamples = description.samples;
    input.numFrags = description.fragments;
    input.pitchInElement = description.pitch;
    ADDR2_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT output{};
    output.size = sizeof(output);
    auto result = Addr2ComputeSurfaceAddrFromCoord(handle, &input, &output);
    if (result != ADDR_OK) throw std::runtime_error("AddrLib surface address calculation failed: " + std::to_string(result));
    return {output.addr, output.bitPosition};
}
