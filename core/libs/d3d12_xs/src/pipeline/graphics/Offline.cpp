#include "Offline.hpp"

AnyBox::Graphics::OfflineGraphicsPipeline::OfflineGraphicsPipeline(Device& device, const RootSignature& root, const GraphicsPipelineDescription& description, const Shader::GraphicsArtifact& vertex, const std::optional<Shader::GraphicsArtifact>& pixel) : vertex(vertex, root, 1), pixel(pixel ? std::optional<OfflineGraphicsStage>(std::in_place, *pixel, root, 5) : std::nullopt), program(device.GetGpuContext(), vertex, pixel ? &*pixel : nullptr), state(device.GetGpuContext(), description) {
    state.SetProgram(program.GetLayout(), program.GetStages());
    auto add = [&](D3D_PRIMITIVE_TOPOLOGY source, VkPrimitiveTopology target) {
        state.SetTopology(target);
        pipelines.emplace_back(source, std::make_unique<Vulkan::GraphicsPipeline>(device.GetGpuContext(), state.GetDescription()));
    };
    switch (description.state.PrimitiveTopologyType) {
        case D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT: add(D3D_PRIMITIVE_TOPOLOGY_POINTLIST, VK_PRIMITIVE_TOPOLOGY_POINT_LIST); break;
        case D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE:
            add(D3D_PRIMITIVE_TOPOLOGY_LINELIST, VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
            add(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
            break;
        case D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE:
            add(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
            add(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
            break;
        default: throw std::runtime_error("Offline graphics primitive topology class is not implemented");
    }
}

const AnyBox::Vulkan::GraphicsPipeline& AnyBox::Graphics::OfflineGraphicsPipeline::GetPipeline(D3D_PRIMITIVE_TOPOLOGY topology) const {
    for (const auto& entry : pipelines) if (entry.first == topology) return *entry.second;
    throw std::invalid_argument("Draw topology is incompatible with the offline graphics pipeline");
}
