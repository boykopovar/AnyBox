#include <anybox/shader/NativePixel.hpp>
#include <anybox/shader/ShaderArtifactBuilder.hpp>
#include <anybox/shader/runtime/GraphicsProgram.hpp>
#include <anybox/vulkan/Framebuffer.hpp>
#include <anybox/vulkan/ImageView.hpp>
#include "../buffer/Target.hpp"
#include "graphics/State.hpp"
#include "Commands.hpp"
#include <cmath>
#include <cstring>

namespace {
void DrawInterpolation(AnyBox::Vulkan::Context& context, bool linear, std::uint32_t wave) {
    using namespace AnyBox;
    using namespace Shader;
    using namespace ShaderRecompiler;
    using namespace Vulkan;
    using namespace Vulkan::Tests;
    auto target = BufferTests::Target();
    target.subgroupSize = context.GetSubgroupProperties().subgroupSize;
    ShaderPixelInputInfo input;
    input.inputNum = 1;
    input.targetOutputMode[0] = 1;
    input.psInputVgpr[static_cast<std::uint32_t>(linear ? PixelInput::LinearCenter : PixelInput::PerspectiveCenter)] = 0;
    std::array<std::uint32_t, 5> words{0xC8080000, 0xC8090001, 0xF8001801, 2, 0xBF810000};
    NativeFrontend frontend(std::as_bytes(std::span(words)), wave);
    Graphics::Container::NativeStageState native{Graphics::Container::NativeHardwareStage::Pixel, 0xC0, true, false};
    auto translated = LowerNativePixel(frontend, native, input, 0, target);
    auto compiled = CompilePixel(translated, input, target, {0, 0, 0, 128});
    auto pixel = MakeGraphicsArtifact(std::move(compiled), {GraphicsArtifactStage::Pixel, {}, {}}, wave, target.subgroupSize);
    GraphicsArtifact vertex;
    vertex.waveSize = vertex.hostSubgroupSize = target.subgroupSize;
    auto qualifier = linear ? "noperspective" : "smooth";
    auto source = std::string("#version 450\nlayout(location=0) ") + qualifier + R"( out vec4 color;
void main() {
    vec2 positions[3] = vec2[3](vec2(-1,-1),vec2(3,-1),vec2(-1,3));
    float w = gl_VertexIndex == 1 ? 2.0 : 1.0;
    gl_Position = vec4(positions[gl_VertexIndex] * w,0,w);
    color = vec4(gl_VertexIndex == 1 ? 1.0 : 0.0,0,0,1);
})";
    vertex.code = CompileShader(source.c_str(), VK_SHADER_STAGE_VERTEX_BIT);
    Runtime::GraphicsProgram program(context, vertex, &pixel);

    constexpr VkExtent2D extent{16, 16};
    ImageDescription imageDescription;
    imageDescription.format = VK_FORMAT_R32_SFLOAT;
    imageDescription.extent = {extent.width, extent.height, 1};
    imageDescription.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    Image image(context, imageDescription);
    ImageView view(image, {});
    VkAttachmentDescription attachment{};
    attachment.format = imageDescription.format;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    VkAttachmentReference color{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color;
    std::array<VkSubpassDependency, 2> dependencies{{{VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0}, {0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, 0}}};
    RenderPass pass(context, std::span(&attachment, 1), std::span(&subpass, 1), dependencies);
    std::array attachments{view.GetHandle()};
    Framebuffer framebuffer(pass, attachments, extent);
    auto stages = program.GetStages();
    GraphicsState state(program.GetLayout(), pass.GetHandle(), stages[0].module, stages[1].module, extent);
    GraphicsPipeline pipeline(context, state.GetDescription());
    auto readback = Buffer::Allocate(context, extent.width * extent.height * sizeof(float), VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    Commands commands(context);
    auto buffer = commands.Begin();
    VkRenderPassBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    begin.renderPass = pass.GetHandle();
    begin.framebuffer = framebuffer.GetHandle();
    begin.renderArea.extent = extent;
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
    for (std::uint32_t y = 0; y < extent.height; ++y) {
        for (std::uint32_t x = 0; x < extent.width; ++x) {
            float value;
            std::memcpy(&value, bytes.data() + (y * extent.width + x) * sizeof(float), sizeof(float));
            auto u = (static_cast<float>(x) + 0.5f) / 32.0f;
            auto expected = linear ? u : u / (2.0f - u);
            Require(std::isfinite(value) && std::abs(value - expected) < 0.00001f, "Native pixel interpolation returned an incorrect value");
        }
    }
}
}

void CheckNativePixelInterpolation(AnyBox::Vulkan::Context& context) {
    for (auto wave : {32u, 64u}) for (auto linear : {false, true}) DrawInterpolation(context, linear, wave);
}
