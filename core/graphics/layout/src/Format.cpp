#include <anybox/layout/Texture.hpp>
#include <stdexcept>

std::uint32_t AnyBox::Layout::TextureBitsPerPixel(std::uint32_t format) {
    if (format >= 1 && format <= 4) return 128;
    if (format >= 9 && format <= 18) return 64;
    if ((format >= 23 && format <= 43) || format == 67 || (format >= 87 && format <= 93) || format == 189 || format == 191) return 32;
    if ((format >= 48 && format <= 59) || format == 85 || format == 86) return 16;
    if ((format >= 60 && format <= 65) || format == 190) return 8;
    throw std::runtime_error("Texture allocation format is not implemented");
}
