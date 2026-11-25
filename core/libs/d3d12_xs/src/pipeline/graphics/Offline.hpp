#pragma once

#include "State.hpp"
#include "../../root/Signature.hpp"
#include <anybox/shader/RootArgumentWriter.hpp>
#include <anybox/shader/runtime/GraphicsProgram.hpp>
#include <optional>

namespace AnyBox::Graphics {
class OfflineGraphicsStage {
public:
    OfflineGraphicsStage(const Shader::GraphicsArtifact& artifact, const RootSignature& root, std::uint32_t visibility) : writer(Shader::RootArgumentPlan(artifact.buffers, root.GetLayout(), 0, visibility), artifact.userRegisters), pushConstantOffset(artifact.pushConstantOffset), pushConstantSize(artifact.pushConstantSize), pushDataStart(artifact.pushDataStart), volatileTables(writer.RequiresVolatileTables(root.GetLayout())) {}
    const Shader::RootArgumentWriter& GetArgumentWriter() const noexcept { return writer; }
    std::uint32_t GetPushConstantOffset() const noexcept { return pushConstantOffset; }
    std::uint32_t GetPushConstantSize() const noexcept { return pushConstantSize; }
    std::uint32_t GetPushDataStart() const noexcept { return pushDataStart; }
    bool HasVolatileTables() const noexcept { return volatileTables; }

private:
    Shader::RootArgumentWriter writer;
    std::uint32_t pushConstantOffset;
    std::uint32_t pushConstantSize;
    std::uint32_t pushDataStart;
    bool volatileTables;
};

class OfflineGraphicsPipeline {
public:
    OfflineGraphicsPipeline(Device& device, const RootSignature& root, const GraphicsPipelineDescription& description, const Shader::GraphicsArtifact& vertex, const std::optional<Shader::GraphicsArtifact>& pixel);
    const Shader::Runtime::GraphicsProgram& GetProgram() const noexcept { return program; }
    const Vulkan::RenderPass& GetRenderPass() const noexcept { return state.GetRenderPass(); }
    const Vulkan::GraphicsPipeline& GetPipeline(D3D_PRIMITIVE_TOPOLOGY topology) const;
    const OfflineGraphicsStage& GetVertex() const noexcept { return vertex; }
    const std::optional<OfflineGraphicsStage>& GetPixel() const noexcept { return pixel; }

private:
    OfflineGraphicsStage vertex;
    std::optional<OfflineGraphicsStage> pixel;
    Shader::Runtime::GraphicsProgram program;
    GraphicsFixedState state;
    std::vector<std::pair<D3D_PRIMITIVE_TOPOLOGY, std::unique_ptr<Vulkan::GraphicsPipeline>>> pipelines;
};
}
