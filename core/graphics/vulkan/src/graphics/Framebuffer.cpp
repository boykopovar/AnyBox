#include <anybox/vulkan/Framebuffer.hpp>

AnyBox::Vulkan::Framebuffer::Framebuffer(RenderPass& pass, std::span<const VkImageView> attachments, VkExtent2D extent, std::uint32_t layers) : context(pass.GetContext()) {
    const auto& limits = context.GetProperties().limits;
    if (attachments.size() != pass.GetAttachmentCount()) throw std::invalid_argument("Vulkan framebuffer attachment count differs from its render pass");
    if (extent.width == 0 || extent.height == 0 || layers == 0 || extent.width > limits.maxFramebufferWidth || extent.height > limits.maxFramebufferHeight || layers > limits.maxFramebufferLayers) throw std::invalid_argument("Vulkan framebuffer dimensions exceed device limits");
    for (auto view : attachments) if (!view) throw std::invalid_argument("Vulkan framebuffer image view is null");
    VkFramebufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass = pass.GetHandle();
    info.attachmentCount = static_cast<std::uint32_t>(attachments.size());
    info.pAttachments = attachments.data();
    info.width = extent.width;
    info.height = extent.height;
    info.layers = layers;
    Check(context.GetFunctions().createFramebuffer(context.GetDevice(), &info, nullptr, &framebuffer), "Create Vulkan framebuffer");
}

AnyBox::Vulkan::Framebuffer::~Framebuffer() {
    context.GetFunctions().destroyFramebuffer(context.GetDevice(), framebuffer, nullptr);
}
