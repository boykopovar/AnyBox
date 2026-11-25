#include <anybox/address/Context.hpp>
#include <vector>
#include <stdexcept>

void CheckUnsupportedCopyProfile(const AnyBox::Address::Context& context) {
    AnyBox::Address::SurfaceDescription description;
    description.width = 256;
    description.height = 128;
    description.slices = description.mipLevels = description.samples = description.fragments = 1;
    description.bitsPerElement = 8;
    description.swizzleMode = 27;
    description.resourceType = 1;
    std::vector<std::byte> surface(65536);
    std::vector<std::byte> linear(32768);
    try { context.CopyToSurface(description, surface.data(), linear.data(), {0, 0, 256, 32768}); }
    catch (const std::runtime_error&) { return; }
    throw std::runtime_error("Surface copy accepted an unsupported hardware profile");
}
