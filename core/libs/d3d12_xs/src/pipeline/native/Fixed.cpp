#include "Fixed.hpp"
#include <stdexcept>

namespace {
D3D12_DEPTH_STENCILOP_DESC Face(const AnyBox::Graphics::Container::SerializedStencilFace& source) {
    if (source.fail < 1 || source.fail > 8 || source.pass < 1 || source.pass > 8 || source.depthFail < 1 || source.depthFail > 8) throw std::runtime_error("Native extended stencil operations are not implemented for graphics pipelines");
    return {static_cast<D3D12_STENCIL_OP>(source.fail), static_cast<D3D12_STENCIL_OP>(source.depthFail), static_cast<D3D12_STENCIL_OP>(source.pass), static_cast<D3D12_COMPARISON_FUNC>(source.comparison)};
}
}

AnyBox::Graphics::GraphicsPipelineDescription AnyBox::Graphics::MakeSerializedFixedDescription(const Container::SerializedGraphicsFixedState& source) {
    if (source.alphaToCoverage > 1) throw std::runtime_error("Native extended alpha-to-coverage mode is not implemented for graphics pipelines");
    if (source.depth.boundsEnable) throw std::runtime_error("Native serialized depth bounds ranges are not implemented for graphics pipelines");
    const auto& depth = source.depth;
    if (depth.stencilEnable && (depth.reference != 0 || (depth.separateBackFace && depth.backReference != 0))) throw std::runtime_error("Native serialized stencil reference initialization is not implemented");
    if (depth.stencilEnable && depth.separateBackFace && (depth.readMask != depth.backReadMask || depth.writeMask != depth.backWriteMask)) throw std::runtime_error("Native per-face stencil masks are not implemented for graphics pipelines");
    GraphicsPipelineDescription result{};
    auto& state = result.state;
    state.SampleMask = source.sampleMask;
    state.NumRenderTargets = source.targetCount;
    for (std::size_t index = 0; index < source.formats.size(); ++index) state.RTVFormats[index] = static_cast<DXGI_FORMAT>(source.formats[index]);
    state.DSVFormat = static_cast<DXGI_FORMAT>(source.depthFormat);
    state.SampleDesc = {source.samples, source.sampleQuality};
    state.PrimitiveTopologyType = static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(source.topology);
    state.IBStripCutValue = static_cast<D3D12_INDEX_BUFFER_STRIP_CUT_VALUE>(source.stripCut);
    state.Flags = static_cast<D3D12_PIPELINE_STATE_FLAGS>(source.flags);
    const auto& rasterizer = source.rasterizer;
    state.RasterizerState = {static_cast<D3D12_FILL_MODE>(rasterizer.fill), static_cast<D3D12_CULL_MODE>(rasterizer.cull), rasterizer.frontCounterClockwise, rasterizer.depthBias, rasterizer.depthBiasClamp, rasterizer.slopeScaledDepthBias, rasterizer.depthClip, rasterizer.multisample, rasterizer.antialiasedLine, rasterizer.forcedSampleCount, static_cast<D3D12_CONSERVATIVE_RASTERIZATION_MODE>(rasterizer.conservative)};
    state.DepthStencilState.DepthEnable = depth.depthEnable;
    state.DepthStencilState.DepthWriteMask = depth.depthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    state.DepthStencilState.DepthFunc = static_cast<D3D12_COMPARISON_FUNC>(depth.comparison);
    state.DepthStencilState.StencilEnable = depth.stencilEnable;
    state.DepthStencilState.StencilReadMask = depth.readMask;
    state.DepthStencilState.StencilWriteMask = depth.writeMask;
    if (depth.stencilEnable) {
        state.DepthStencilState.FrontFace = Face(depth.front);
        state.DepthStencilState.BackFace = Face(depth.separateBackFace ? depth.back : depth.front);
    }
    state.BlendState.AlphaToCoverageEnable = source.alphaToCoverage != 0;
    state.BlendState.IndependentBlendEnable = source.independentBlend;
    for (std::size_t index = 0; index < source.targets.size(); ++index) {
        const auto& input = source.targets[index];
        state.BlendState.RenderTarget[index] = {input.enable, input.logicEnable, static_cast<D3D12_BLEND>(input.source), static_cast<D3D12_BLEND>(input.destination), static_cast<D3D12_BLEND_OP>(input.operation), static_cast<D3D12_BLEND>(input.sourceAlpha), static_cast<D3D12_BLEND>(input.destinationAlpha), static_cast<D3D12_BLEND_OP>(input.alphaOperation), static_cast<D3D12_LOGIC_OP>(input.logicOperation), input.writeMask};
    }
    return result;
}
