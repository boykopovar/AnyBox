#pragma once

#include "Context.hpp"
#include <span>

namespace AnyBox::Vulkan {
class RenderPass {
public:
    RenderPass(Context& context, std::span<const VkAttachmentDescription> attachments, std::span<const VkSubpassDescription> subpasses, std::span<const VkSubpassDependency> dependencies = {});
    ~RenderPass();
    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;
    VkRenderPass GetHandle() const noexcept { return pass; }
    Context& GetContext() const noexcept { return context; }
    std::uint32_t GetAttachmentCount() const noexcept { return attachmentCount; }

private:
    Context& context;
    VkRenderPass pass = VK_NULL_HANDLE;
    std::uint32_t attachmentCount;
};
}
