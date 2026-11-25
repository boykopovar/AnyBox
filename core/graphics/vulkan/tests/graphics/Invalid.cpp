#include "../Tests.hpp"
#include <anybox/vulkan/GraphicsPipeline.hpp>
#include <anybox/vulkan/Framebuffer.hpp>
#include <anybox/vulkan/PipelineLayout.hpp>
#include <anybox/vulkan/ShaderModule.hpp>

namespace {
template<class TCallable>
void Reject(TCallable&& callable) {
    bool caught = false;
    try { callable(); } catch (const std::exception&) { caught = true; }
    AnyBox::Vulkan::Tests::Require(caught, "Invalid graphics object was accepted");
}
}

void CheckInvalidGraphics(AnyBox::Vulkan::Context& context, AnyBox::Vulkan::RenderPass& pass, const VkGraphicsPipelineCreateInfo& description) {
    using namespace AnyBox::Vulkan;
    Reject([&] { ShaderModule invalid(context, {}); });
    std::array<VkPushConstantRange, 2> duplicated{{{VK_SHADER_STAGE_VERTEX_BIT, 0, 4}, {VK_SHADER_STAGE_VERTEX_BIT, 4, 4}}};
    Reject([&] { PipelineLayout invalid(context, duplicated); });
    std::array<VkPushConstantRange, 1> oversized{{{VK_SHADER_STAGE_VERTEX_BIT, UINT32_MAX, 4}}};
    Reject([&] { PipelineLayout invalid(context, oversized); });
    Reject([&] { RenderPass invalid(context, {}, {}); });
    Reject([&] { Framebuffer invalid(pass, {}, {16,16}); });
    std::array<VkImageView, 1> missing{};
    Reject([&] { Framebuffer invalid(pass, missing, {16,16}); });
    auto incomplete = description;
    incomplete.pViewportState = nullptr;
    Reject([&] { GraphicsPipeline invalid(context, incomplete); });
    auto derived = description;
    derived.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
    Reject([&] { GraphicsPipeline invalid(context, derived); });
    std::array stages{description.pStages[0], description.pStages[0]};
    auto duplicateStages = description;
    duplicateStages.pStages = stages.data();
    Reject([&] { GraphicsPipeline invalid(context, duplicateStages); });
}
