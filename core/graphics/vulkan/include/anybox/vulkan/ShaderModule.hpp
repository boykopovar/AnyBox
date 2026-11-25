#pragma once

#include "Context.hpp"
#include <span>
#include <string_view>

namespace AnyBox::Vulkan {
class ShaderModule {
public:
    ShaderModule(Context& context, std::span<const std::uint32_t> code, std::string_view entryPoint = "main");
    ~ShaderModule();
    ShaderModule(const ShaderModule&) = delete;
    ShaderModule& operator=(const ShaderModule&) = delete;
    VkShaderModule GetHandle() const noexcept { return module; }
    VkPushConstantRange GetPushConstants() const noexcept { return constants; }

private:
    Context& context;
    VkShaderModule module = VK_NULL_HANDLE;
    VkPushConstantRange constants{};
};
}
