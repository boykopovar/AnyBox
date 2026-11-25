#pragma once

#include <anybox/shader/GraphicsArtifact.hpp>
#include <anybox/vulkan/ShaderModule.hpp>
#include <anybox/vulkan/PipelineLayout.hpp>
#include <memory>

namespace AnyBox::Shader::Runtime {
class GraphicsProgram {
public:
    GraphicsProgram(Vulkan::Context& context, const GraphicsArtifact& vertex, const GraphicsArtifact* pixel = nullptr);
    ~GraphicsProgram();
    GraphicsProgram(const GraphicsProgram&) = delete;
    GraphicsProgram& operator=(const GraphicsProgram&) = delete;
    VkPipelineLayout GetLayout() const noexcept { return layout->GetHandle(); }
    VkDescriptorSetLayout GetDescriptorLayout() const noexcept { return descriptorLayout; }
    std::uint32_t GetDescriptorMask() const noexcept { return descriptorMask; }
    std::span<const VkPipelineShaderStageCreateInfo> GetStages() const noexcept { return stages; }
    std::span<const VkPushConstantRange> GetPushConstants() const noexcept { return constants; }
    Vulkan::Context& GetContext() const noexcept { return context; }

private:
    Vulkan::Context& context;
    VkDescriptorSetLayout descriptorLayout = VK_NULL_HANDLE;
    std::uint32_t descriptorMask = 0;
    std::vector<std::unique_ptr<Vulkan::ShaderModule>> modules;
    std::vector<VkPipelineShaderStageCreateInfo> stages;
    std::vector<VkPushConstantRange> constants;
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    std::unique_ptr<Vulkan::PipelineLayout> layout;
    void addStage(const GraphicsArtifact& artifact);
};
}
