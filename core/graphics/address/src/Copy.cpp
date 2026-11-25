#include <anybox/address/Context.hpp>
#include <addrinterface.h>
#include <algorithm>
#include <stdexcept>
#include <string>

void AnyBox::Address::Context::CopyToSurface(const SurfaceDescription& description, void* surface, const void* memory, const SurfaceCopy& copy) const {
    copySurface(description, surface, const_cast<void*>(memory), copy, true);
}

void AnyBox::Address::Context::CopyFromSurface(const SurfaceDescription& description, void* memory, const void* surface, const SurfaceCopy& copy) const {
    copySurface(description, const_cast<void*>(surface), memory, copy, false);
}

void AnyBox::Address::Context::copySurface(const SurfaceDescription& description, void* surface, void* memory, const SurfaceCopy& copy, bool toSurface) const {
    if (!surfaceCopy) throw std::runtime_error("Surface copy hardware profile is not implemented");
    if (!surface || !memory) throw std::invalid_argument("Surface copy memory pointers are null");
    if (description.swizzleMode != 0 && description.swizzleMode != 5 && description.swizzleMode != 21 && description.swizzleMode != 27) throw std::runtime_error("Surface addressing swizzle mode is not implemented");
    if ((description.flags & ~0x10601u) != 0) throw std::runtime_error("Surface addressing flags are not implemented");
    if (description.resourceType != 1 || description.format != 0 || description.samples != 1 || description.fragments != 1) throw std::runtime_error("Surface copy resource type, format or samples are not implemented");
    if (description.width == 0 || description.height == 0 || description.width > 16384 || description.height > 16384 || description.slices > 2048 || description.slices == 0 || description.mipLevels == 0 || description.mipLevels > 16 || copy.mip >= description.mipLevels || copy.slice >= description.slices) throw std::invalid_argument("Surface copy dimensions or subresource are invalid");
    if (description.bitsPerElement < 8 || description.bitsPerElement > 128 || (description.bitsPerElement & (description.bitsPerElement - 1)) != 0) throw std::runtime_error("Surface copy element size is not implemented");
    auto width = std::max(1u, description.width >> copy.mip);
    auto height = std::max(1u, description.height >> copy.mip);
    if (copy.rowPitch < static_cast<std::uint64_t>(width) * (description.bitsPerElement / 8)) throw std::invalid_argument("Surface copy row pitch does not cover its width");
    ADDR2_COPY_MEMSURFACE_INPUT input{};
    input.size = sizeof(input);
    input.swizzleMode = static_cast<AddrSwizzleMode>(description.swizzleMode);
    input.resourceType = static_cast<AddrResourceType>(description.resourceType);
    input.flags.value = description.flags;
    input.bpp = description.bitsPerElement;
    input.unAlignedDims = {description.width, description.height, description.slices};
    input.numMipLevels = description.mipLevels;
    input.numSamples = description.samples;
    input.pitchInElement = description.pitch;
    input.pMappedSurface = surface;
    ADDR2_COPY_MEMSURFACE_REGION region{};
    region.size = sizeof(region);
    region.mipId = copy.mip;
    region.slice = copy.slice;
    region.copyDims = {width, height, 1};
    region.pMem = memory;
    region.memRowPitch = copy.rowPitch;
    region.memSlicePitch = copy.slicePitch;
    auto result = toSurface ? Addr2CopyMemToSurface(handle, &input, &region, 1) : Addr2CopySurfaceToMem(handle, &input, &region, 1);
    if (result != ADDR_OK) throw std::runtime_error("AddrLib surface copy failed: " + std::to_string(result));
}
