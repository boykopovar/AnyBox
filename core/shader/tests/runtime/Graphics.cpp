#include <anybox/shader/runtime/GraphicsProgram.hpp>
#include <anybox/vulkan/RenderPass.hpp>
#include "graphics/State.hpp"
#include "Tests.hpp"

namespace {
template<class TCallable>
void Reject(TCallable&& callable) {
    try { callable(); } catch (const std::exception&) { return; }
    throw std::runtime_error("Offline graphics program accepted unsupported shader data");
}
}

void CheckGraphicsProgram(AnyBox::Vulkan::Context& context) {
    using namespace AnyBox;
    using namespace Shader;
    using Vulkan::Tests::Require;
    GraphicsArtifact vertex;
    vertex.hostSubgroupSize = context.GetSubgroupProperties().subgroupSize;
    vertex.waveSize = vertex.hostSubgroupSize;
    vertex.code = Vulkan::Tests::CompileShader("#version 450\nlayout(push_constant) uniform Args { uint words[32]; } args; void main() { gl_Position = vec4(float(args.words[0]),0,0,1); }", VK_SHADER_STAGE_VERTEX_BIT);
    vertex.pushConstantSize = 4;
    vertex.pushDataStart = 0;
    vertex.memoryOffset = 1;
    vertex.userRegisters = {0};
    vertex.descriptors = {{59, {}}};
    GraphicsArtifact pixel;
    pixel.stage = GraphicsArtifactStage::Pixel;
    pixel.hostSubgroupSize = vertex.hostSubgroupSize;
    pixel.waveSize = vertex.waveSize;
    pixel.code = Vulkan::Tests::CompileShader("#version 450\nlayout(location=0) out vec4 color; void main() { color = vec4(0,1,0,1); }", VK_SHADER_STAGE_FRAGMENT_BIT);
    pixel.descriptors = {{59, {}}};
    Runtime::GraphicsProgram program(context, vertex, &pixel);
    Require(program.GetStages().size() == 2 && program.GetDescriptorLayout() && program.GetDescriptorMask() == 1, "Offline graphics program lost its stages or shared descriptors");
    auto constants = program.GetPushConstants();
    Require(constants.size() == 1 && constants[0].offset == 0 && constants[0].size == 128 && constants[0].stageFlags == VK_SHADER_STAGE_VERTEX_BIT, "Offline graphics program did not reserve the full declared push constant block");
    Runtime::GraphicsProgram vertexOnly(context, vertex);
    Require(vertexOnly.GetStages().size() == 1, "Vertex-only graphics program added a pixel stage");

    VkAttachmentDescription attachment{};
    attachment.format = VK_FORMAT_R8G8B8A8_UNORM;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkAttachmentReference reference{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &reference;
    Vulkan::RenderPass pass(context, std::span(&attachment, 1), std::span(&subpass, 1));
    auto stages = program.GetStages();
    Vulkan::Tests::GraphicsState state(program.GetLayout(), pass.GetHandle(), stages[0].module, stages[1].module, {16, 16});
    auto description = state.GetDescription();
    description.stageCount = static_cast<std::uint32_t>(stages.size());
    description.pStages = stages.data();
    Vulkan::GraphicsPipeline pipeline(context, description);

    auto changed = pixel;
    changed.layoutHash[0] ^= std::byte{1};
    Reject([&] { Runtime::GraphicsProgram invalid(context, vertex, &changed); });
    Reject([&] { Runtime::GraphicsProgram invalid(context, pixel); });
    changed = vertex;
    changed.descriptors = {{62, {}}};
    Reject([&] { Runtime::GraphicsProgram invalid(context, changed); });
    changed = vertex;
    changed.hostSubgroupSize = vertex.hostSubgroupSize == 32 ? 64 : 32;
    Reject([&] { Runtime::GraphicsProgram invalid(context, changed); });
    changed = vertex;
    changed.memoryOffsetCount = 1;
    changed.pushConstantSize = 8;
    Reject([&] { Runtime::GraphicsProgram invalid(context, changed); });
    changed = vertex;
    changed.pushDataStart = UINT32_MAX;
    Reject([&] { Runtime::GraphicsProgram invalid(context, changed); });
    changed = vertex;
    changed.code.insert(changed.code.begin() + 5, {0x00020011, 10});
    Reject([&] { Runtime::GraphicsProgram invalid(context, changed); });
}
