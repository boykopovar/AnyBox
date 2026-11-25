#include "State.hpp"

AnyBox::Graphics::GraphicsFixedState::GraphicsFixedState(Vulkan::Context& context, const GraphicsPipelineDescription& source) : context(context) {
    const auto& state = source.state;
    if (state.InputLayout.NumElements != 0) throw std::runtime_error("Offline graphics input assembler attributes are not implemented");
    if (state.StreamOutput.NumEntries != 0 || state.StreamOutput.NumStrides != 0 || state.StreamOutput.RasterizedStream != 0) throw std::runtime_error("Offline graphics stream output is not implemented");
    if (state.DS.BytecodeLength || state.HS.BytecodeLength || state.GS.BytecodeLength) throw std::runtime_error("Offline graphics tessellation and geometry stages are not implemented");
    if (state.IBStripCutValue != D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED) throw std::runtime_error("Offline graphics index strip cut is not implemented");
    if (source.viewInstancing.ViewInstanceCount || source.viewInstancing.Flags) throw std::runtime_error("Offline graphics view instancing is not implemented");
    if (state.NodeMask > 1 || (static_cast<std::uint32_t>(state.Flags) & ~1u) != 0) throw std::invalid_argument("Offline graphics node mask or flags are invalid");
    if (state.CachedPSO.CachedBlobSizeInBytes) throw std::runtime_error("Xbox graphics pipeline cache loading is not implemented");
    if (state.NumRenderTargets > colors.size() || state.NumRenderTargets > context.GetProperties().limits.maxColorAttachments) throw std::invalid_argument("Offline graphics render target count exceeds the device limit");
    if (state.SampleDesc.Quality != 0) throw std::runtime_error("Offline graphics sample quality selection is not implemented");
    auto samples = state.SampleDesc.Count;
    if (samples == 0 || samples > 32 || (samples & (samples - 1)) != 0) throw std::invalid_argument("Offline graphics sample count is invalid");
    if (state.PrimitiveTopologyType < D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT || state.PrimitiveTopologyType > D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE) throw std::runtime_error("Offline graphics primitive topology class is not implemented");
    initializeRasterizer(state.RasterizerState);
    initializeBlend(state.BlendState, state.NumRenderTargets);
    initializeDepth(state.DepthStencilState, source.depthBounds);
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = static_cast<VkSampleCountFlagBits>(samples);
    multisample.alphaToCoverageEnable = state.BlendState.AlphaToCoverageEnable != FALSE;
    sampleMask = state.SampleMask;
    multisample.pSampleMask = &sampleMask;
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    viewports.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewports.viewportCount = viewports.scissorCount = 1;
    dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic.dynamicStateCount = static_cast<std::uint32_t>(dynamicStates.size());
    dynamic.pDynamicStates = dynamicStates.data();
    initializePass(state);
    description.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    description.pVertexInputState = &vertexInput;
    description.pInputAssemblyState = &assembly;
    description.pViewportState = &viewports;
    description.pRasterizationState = &rasterizer;
    description.pMultisampleState = &multisample;
    description.pColorBlendState = &blend;
    description.pDepthStencilState = &depth;
    description.pDynamicState = &dynamic;
    description.renderPass = pass->GetHandle();
    description.basePipelineIndex = -1;
}

void AnyBox::Graphics::GraphicsFixedState::SetProgram(VkPipelineLayout layout, std::span<const VkPipelineShaderStageCreateInfo> stages) {
    if (!layout || stages.empty() || stages.size() > 2) throw std::invalid_argument("Offline graphics program stages or layout are invalid");
    description.layout = layout;
    description.stageCount = static_cast<std::uint32_t>(stages.size());
    description.pStages = stages.data();
}
