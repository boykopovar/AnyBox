#pragma once

#include "View.hpp"
#include <array>

namespace AnyBox::Graphics::Container {
class FspxView {
public:
    explicit FspxView(std::span<const std::byte> data);
    std::span<const std::byte> GetDispatchState() const noexcept { return parts[0]; }
    std::span<const std::byte> GetAuxiliaryState() const noexcept { return parts[1]; }
    std::span<const std::byte> GetCode() const noexcept { return parts[2]; }
    std::span<const std::byte> GetRecords() const noexcept { return parts[3]; }

private:
    std::array<std::span<const std::byte>, 4> parts{};
};

std::vector<std::byte> BuildFspx(std::span<const Part> parts);
}
