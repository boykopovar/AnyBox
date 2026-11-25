#include "State.hpp"
#include "../../resource/Texture.hpp"

namespace {
VkFormat DepthFormat(DXGI_FORMAT value) {
    switch (value) {
        case DXGI_FORMAT_D32_FLOAT: return VK_FORMAT_D32_SFLOAT;
        case DXGI_FORMAT_D16_UNORM: return VK_FORMAT_D16_UNORM;
        case DXGI_FORMAT_D24_UNORM_S8_UINT: return VK_FORMAT_D24_UNORM_S8_UINT;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return VK_FORMAT_D32_SFLOAT_S8_UINT;
        default: throw std::runtime_error("Offline graphics depth stencil format is not implemented");
    }
}
}

void AnyBox::Graphics::GraphicsFixedState::initializePass(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& source) {
    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> references;
    const auto& limits = context.GetProperties().limits;
    auto samples = static_cast<VkSampleCountFlagBits>(source.SampleDesc.Count);
    auto add = [&](VkFormat format, VkImageLayout layout) {
        VkAttachmentDescription attachment{};
        attachment.format = format;
        attachment.samples = samples;
        attachment.loadOp = attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachment.storeOp = attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.initialLayout = attachment.finalLayout = layout;
        attachments.push_back(attachment);
    };
    if (source.NumRenderTargets && (limits.framebufferColorSampleCounts & samples) == 0) throw std::runtime_error("Offline graphics color sample count is unsupported by the device");
    for (std::uint32_t index = 0; index < source.NumRenderTargets; ++index) {
        auto format = TextureFormat(source.RTVFormats[index]).format;
        references.push_back({index, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
        add(format, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }
    if (source.NumRenderTargets == 0 && source.DSVFormat == DXGI_FORMAT_UNKNOWN && (limits.framebufferNoAttachmentsSampleCounts & samples) == 0) throw std::runtime_error("Offline graphics attachment-free sample count is unsupported by the device");
    VkAttachmentReference depthReference{VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    if (source.DSVFormat != DXGI_FORMAT_UNKNOWN) {
        if (source.DepthStencilState.StencilEnable && source.DSVFormat != DXGI_FORMAT_D24_UNORM_S8_UINT && source.DSVFormat != DXGI_FORMAT_D32_FLOAT_S8X24_UINT) throw std::invalid_argument("Offline graphics stencil testing requires a stencil attachment format");
        if (source.DepthStencilState.StencilEnable && (limits.framebufferStencilSampleCounts & samples) == 0) throw std::runtime_error("Offline graphics stencil sample count is unsupported by the device");
        if ((limits.framebufferDepthSampleCounts & samples) == 0) throw std::runtime_error("Offline graphics depth sample count is unsupported by the device");
        depthReference.attachment = static_cast<std::uint32_t>(attachments.size());
        add(DepthFormat(source.DSVFormat), depthReference.layout);
    } else if (source.DepthStencilState.DepthEnable || source.DepthStencilState.StencilEnable || depth.depthBoundsTestEnable) throw std::invalid_argument("Offline graphics depth stencil state requires an attachment format");
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<std::uint32_t>(references.size());
    subpass.pColorAttachments = references.data();
    if (depthReference.attachment != VK_ATTACHMENT_UNUSED) subpass.pDepthStencilAttachment = &depthReference;
    pass = std::make_unique<Vulkan::RenderPass>(context, attachments, std::span(&subpass, 1));
}
