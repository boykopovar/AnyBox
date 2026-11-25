#include "State.hpp"
#include "../Commands.hpp"
#include <anybox/vulkan/Framebuffer.hpp>
#include <anybox/vulkan/ImageView.hpp>
#include <anybox/vulkan/PipelineLayout.hpp>
#include <anybox/vulkan/ShaderModule.hpp>
#include <iostream>

void CheckSpirvPushConstants();
void CheckComputeDeclaredPushConstants(AnyBox::Vulkan::Context& context);
void CheckInvalidGraphics(AnyBox::Vulkan::Context& context, AnyBox::Vulkan::RenderPass& pass, const VkGraphicsPipelineCreateInfo& description);

namespace {
void Draw(AnyBox::Vulkan::Context& context) {
    using namespace AnyBox::Vulkan;
    using namespace AnyBox::Vulkan::Tests;
    constexpr VkExtent2D extent{16, 16};
    ImageDescription imageDescription;
    imageDescription.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageDescription.extent = {extent.width, extent.height, 1};
    imageDescription.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    Image image(context, imageDescription);
    ImageView view(image, {});
    VkAttachmentDescription attachment{};
    attachment.format = imageDescription.format;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    VkAttachmentReference color{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color;
    std::array<VkSubpassDependency, 2> dependencies{};
    dependencies[0] = {VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0};
    dependencies[1] = {0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, 0};
    RenderPass pass(context, std::span(&attachment, 1), std::span(&subpass, 1), dependencies);
    std::array attachments{view.GetHandle()};
    Framebuffer framebuffer(pass, attachments, extent);
    PipelineLayout layout(context);
    auto vertexCode = CompileShader(R"(#version 450
layout(location=0) flat out vec4 color;
void main() {
    vec2 positions[3] = vec2[3](vec2(-1,-1),vec2(3,-1),vec2(-1,3));
    gl_Position = vec4(positions[gl_VertexIndex],0,1);
    color = vec4(0,1,0,1);
})", VK_SHADER_STAGE_VERTEX_BIT);
    auto pixelCode = CompileShader(R"(#version 450
layout(location=0) flat in vec4 color;
layout(location=0) out vec4 outputColor;
void main() { outputColor = color; }
)", VK_SHADER_STAGE_FRAGMENT_BIT);
    ShaderModule vertex(context, vertexCode);
    ShaderModule pixel(context, pixelCode);
    GraphicsState state(layout.GetHandle(), pass.GetHandle(), vertex.GetHandle(), pixel.GetHandle(), extent);
    CheckInvalidGraphics(context, pass, state.GetDescription());
    GraphicsPipeline pipeline(context, state.GetDescription());
    auto readback = Buffer::Allocate(context, extent.width * extent.height * 4, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    Commands commands(context);
    auto buffer = commands.Begin();
    VkClearValue clear{};
    VkRenderPassBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    begin.renderPass = pass.GetHandle();
    begin.framebuffer = framebuffer.GetHandle();
    begin.renderArea.extent = extent;
    begin.clearValueCount = 1;
    begin.pClearValues = &clear;
    const auto& functions = context.GetFunctions();
    functions.beginRenderPass(buffer, &begin, VK_SUBPASS_CONTENTS_INLINE);
    functions.bindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetHandle());
    functions.draw(buffer, 3, 1, 0, 0);
    functions.endRenderPass(buffer);
    VkBufferImageCopy region{};
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageExtent = imageDescription.extent;
    functions.copyImageToBuffer(buffer, image.GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readback.GetHandle(), 1, &region);
    commands.Submit(buffer);
    auto bytes = readback.Map();
    for (std::size_t index = 0; index < bytes.size(); ++index) Require(bytes[index] == (index % 2 == 0 ? std::byte{0} : std::byte{255}), "Vulkan graphics draw returned unexpected pixels");
}
}

int main() {
    try {
        AnyBox::Vulkan::Context context(0);
        std::cout << "Vulkan graphics adapter: " << context.GetProperties().deviceName << '\n';
        Draw(context);
        CheckSpirvPushConstants();
        CheckComputeDeclaredPushConstants(context);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
