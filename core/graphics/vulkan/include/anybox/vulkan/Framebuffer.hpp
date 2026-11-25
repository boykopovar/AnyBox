#pragma once

#include "RenderPass.hpp"

namespace AnyBox::Vulkan {
class Framebuffer {
public:
    Framebuffer(RenderPass& pass, std::span<const VkImageView> attachments, VkExtent2D extent, std::uint32_t layers = 1);
    ~Framebuffer();
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;
    VkFramebuffer GetHandle() const noexcept { return framebuffer; }

private:
    Context& context;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
};
}
