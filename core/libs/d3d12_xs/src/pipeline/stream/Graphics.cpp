#include "Graphics.hpp"
#include <algorithm>

AnyBox::Graphics::GraphicsPipelineDescription AnyBox::Graphics::ReadGraphicsPipeline(const PipelineStream& stream) {
    if (stream.GetKind() != PipelineStreamKind::Graphics) throw std::invalid_argument("Xbox pipeline stream is not a vertex graphics pipeline");
    auto description = DefaultGraphicsPipeline();
    auto& state = description.state;
    if (stream.Has(0)) state.pRootSignature = stream.Read<ID3D12RootSignature*>(0);
    state.VS = stream.GetShader(1);
    state.PS = stream.GetShader(2);
    state.DS = stream.GetShader(3);
    state.HS = stream.GetShader(4);
    state.GS = stream.GetShader(5);
    if (stream.Has(7)) state.StreamOutput = stream.Read<D3D12_STREAM_OUTPUT_DESC>(7);
    if (stream.Has(8)) state.BlendState = stream.Read<D3D12_BLEND_DESC>(8);
    if (stream.Has(9)) state.SampleMask = stream.Read<std::uint32_t>(9);
    if (stream.Has(10)) state.RasterizerState = stream.Read<D3D12_RASTERIZER_DESC>(10);
    if (stream.Has(11)) state.DepthStencilState = stream.Read<D3D12_DEPTH_STENCIL_DESC>(11);
    if (stream.Has(12)) state.InputLayout = stream.Read<D3D12_INPUT_LAYOUT_DESC>(12);
    if (stream.Has(13)) state.IBStripCutValue = stream.Read<D3D12_INDEX_BUFFER_STRIP_CUT_VALUE>(13);
    if (stream.Has(14)) state.PrimitiveTopologyType = stream.Read<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(14);
    if (stream.Has(15)) {
        auto formats = stream.Read<D3D12_RT_FORMAT_ARRAY>(15);
        if (formats.NumRenderTargets > D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT) throw std::invalid_argument("Graphics pipeline render target count exceeds its array");
        state.NumRenderTargets = formats.NumRenderTargets;
        std::copy(std::begin(formats.RTFormats), std::end(formats.RTFormats), std::begin(state.RTVFormats));
    }
    if (stream.Has(16)) state.DSVFormat = stream.Read<DXGI_FORMAT>(16);
    if (stream.Has(17)) state.SampleDesc = stream.Read<DXGI_SAMPLE_DESC>(17);
    if (stream.Has(18)) state.NodeMask = stream.Read<std::uint32_t>(18);
    if (stream.Has(19)) state.CachedPSO = stream.Read<D3D12_CACHED_PIPELINE_STATE>(19);
    if (stream.Has(20)) state.Flags = stream.Read<D3D12_PIPELINE_STATE_FLAGS>(20);
    if (stream.Has(21)) {
        auto depth = stream.Read<D3D12_DEPTH_STENCIL_DESC1>(21);
        state.DepthStencilState.DepthEnable = depth.DepthEnable;
        state.DepthStencilState.DepthWriteMask = depth.DepthWriteMask;
        state.DepthStencilState.DepthFunc = depth.DepthFunc;
        state.DepthStencilState.StencilEnable = depth.StencilEnable;
        state.DepthStencilState.StencilReadMask = depth.StencilReadMask;
        state.DepthStencilState.StencilWriteMask = depth.StencilWriteMask;
        state.DepthStencilState.FrontFace = depth.FrontFace;
        state.DepthStencilState.BackFace = depth.BackFace;
        description.depthBounds = depth.DepthBoundsTestEnable;
    }
    if (stream.Has(22)) description.viewInstancing = stream.Read<D3D12_VIEW_INSTANCING_DESC>(22);
    return description;
}
