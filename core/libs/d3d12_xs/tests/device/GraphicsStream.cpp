#include "StreamFixture.hpp"
#include "../../src/pipeline/stream/Graphics.hpp"

void CheckGraphicsPipelineStream() {
    using namespace AnyBox::Graphics;
    using namespace AnyBox::Graphics::Tests;
    std::array<std::byte, 4> code{};
    D3D12_SHADER_BYTECODE shader{code.data(), code.size()};
    StreamFixture minimal;
    minimal.Append(1, shader);
    auto defaults = ReadGraphicsPipeline(PipelineStream(minimal.GetBytes()));
    Require(defaults.state.VS.pShaderBytecode == code.data() && defaults.state.SampleMask == UINT32_MAX && defaults.state.SampleDesc.Count == 1, "Graphics stream defaults lost shader, samples or sample mask");
    Require(defaults.state.RasterizerState.CullMode == D3D12_CULL_MODE_BACK && defaults.state.RasterizerState.DepthClipEnable && defaults.state.DepthStencilState.DepthEnable, "Graphics stream fixed state defaults changed");
    Require(defaults.state.BlendState.RenderTarget[0].RenderTargetWriteMask == 15 && defaults.state.BlendState.RenderTarget[7].SrcBlend == D3D12_BLEND_ONE, "Graphics stream blend defaults changed");
    D3D12_DEPTH_STENCIL_DESC1 depth{};
    depth.DepthEnable = FALSE;
    depth.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
    depth.DepthBoundsTestEnable = TRUE;
    depth.StencilReadMask = 0x42;
    depth.BackFace.StencilPassOp = D3D12_STENCIL_OP_INVERT;
    minimal.Append(21, depth);
    D3D12_RT_FORMAT_ARRAY formats{};
    formats.NumRenderTargets = 2;
    formats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    formats.RTFormats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    minimal.Append(15, formats);
    minimal.Append(17, DXGI_SAMPLE_DESC{4, 3});
    minimal.Append(9, 0xFF00FF00u);
    minimal.Append(14, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    auto parsed = ReadGraphicsPipeline(PipelineStream(minimal.GetBytes()));
    Require(parsed.depthBounds && !parsed.state.DepthStencilState.DepthEnable && parsed.state.DepthStencilState.DepthFunc == D3D12_COMPARISON_FUNC_GREATER && parsed.state.DepthStencilState.StencilReadMask == 0x42 && parsed.state.DepthStencilState.BackFace.StencilPassOp == D3D12_STENCIL_OP_INVERT, "Graphics stream lost extended depth stencil state");
    Require(parsed.state.NumRenderTargets == 2 && parsed.state.RTVFormats[1] == DXGI_FORMAT_R16G16B16A16_FLOAT && parsed.state.SampleDesc.Count == 4 && parsed.state.SampleDesc.Quality == 3 && parsed.state.SampleMask == 0xFF00FF00u, "Graphics stream lost formats or multisample state");
    StreamFixture compute;
    compute.Append(6, shader);
    Reject<std::invalid_argument>([&] { ReadGraphicsPipeline(PipelineStream(compute.GetBytes())); });
    formats.NumRenderTargets = 9;
    StreamFixture invalid;
    invalid.Append(1, shader);
    invalid.Append(15, formats);
    Reject<std::invalid_argument>([&] { ReadGraphicsPipeline(PipelineStream(invalid.GetBytes())); });
}
