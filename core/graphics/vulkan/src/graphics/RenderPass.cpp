#include <anybox/vulkan/RenderPass.hpp>

AnyBox::Vulkan::RenderPass::RenderPass(Context& context, std::span<const VkAttachmentDescription> attachments, std::span<const VkSubpassDescription> subpasses, std::span<const VkSubpassDependency> dependencies) : context(context) {
    if (attachments.size() > UINT32_MAX || subpasses.empty() || subpasses.size() > UINT32_MAX || dependencies.size() > UINT32_MAX) throw std::invalid_argument("Vulkan render pass array size is invalid");
    for (const auto& subpass : subpasses) {
        if (subpass.flags != 0 || subpass.pipelineBindPoint != VK_PIPELINE_BIND_POINT_GRAPHICS) throw std::runtime_error("Vulkan render pass subpass mode is not implemented");
        if (subpass.colorAttachmentCount > context.GetProperties().limits.maxColorAttachments) throw std::invalid_argument("Vulkan subpass color count exceeds device limits");
        if ((subpass.colorAttachmentCount && !subpass.pColorAttachments) || (subpass.inputAttachmentCount && !subpass.pInputAttachments) || (subpass.preserveAttachmentCount && !subpass.pPreserveAttachments)) throw std::invalid_argument("Vulkan subpass attachment array is null");
    }
    for (const auto& dependency : dependencies) {
        if ((dependency.srcSubpass != VK_SUBPASS_EXTERNAL && dependency.srcSubpass >= subpasses.size()) || (dependency.dstSubpass != VK_SUBPASS_EXTERNAL && dependency.dstSubpass >= subpasses.size())) throw std::invalid_argument("Vulkan render pass dependency has an invalid subpass");
    }
    attachmentCount = static_cast<std::uint32_t>(attachments.size());
    VkRenderPassCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = attachmentCount;
    info.pAttachments = attachments.data();
    info.subpassCount = static_cast<std::uint32_t>(subpasses.size());
    info.pSubpasses = subpasses.data();
    info.dependencyCount = static_cast<std::uint32_t>(dependencies.size());
    info.pDependencies = dependencies.data();
    Check(context.GetFunctions().createRenderPass(context.GetDevice(), &info, nullptr, &pass), "Create Vulkan render pass");
}

AnyBox::Vulkan::RenderPass::~RenderPass() {
    context.GetFunctions().destroyRenderPass(context.GetDevice(), pass, nullptr);
}
