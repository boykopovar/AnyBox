#pragma once

#include "Functions.hpp"
#include <span>
#include <string_view>

namespace AnyBox::Vulkan {
VkPushConstantRange ReadSpirvPushConstants(std::span<const std::uint32_t> code, std::string_view entryPoint = "main");
}
