#include "Tests.hpp"
#include "../../src/pipeline/native/Fixed.hpp"
#include "../../src/pipeline/graphics/State.hpp"
#include "graphics/Fixture.hpp"

void AnyBox::Graphics::Tests::CheckSerializedGraphicsState(Device* device) {
    SerializedGraphicsTests::Fixture fixture;
    fixture.Context(116, 15);
    fixture.Context(120, 0xC0050504);
    auto source = fixture.Read();
    auto description = MakeSerializedFixedDescription(source);
    Require(description.state.NumRenderTargets == 1 && description.state.RTVFormats[0] == DXGI_FORMAT_R8G8B8A8_UNORM, "Native graphics formats were changed");
    GraphicsFixedState fixed(device->GetGpuContext(), description);
    const auto& state = fixed.GetDescription();
    Require(state.pRasterizationState->polygonMode == VK_POLYGON_MODE_FILL && state.pRasterizationState->cullMode == VK_CULL_MODE_NONE, "Native graphics rasterization was changed");
    Require(state.pColorBlendState->attachmentCount == 1 && state.pColorBlendState->pAttachments[0].blendEnable, "Native graphics blending was disabled");
    Require(state.pColorBlendState->pAttachments[0].srcColorBlendFactor == VK_BLEND_FACTOR_SRC_ALPHA && state.pColorBlendState->pAttachments[0].dstColorBlendFactor == VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, "Native graphics blend factors were changed");
    Require(state.pColorBlendState->pAttachments[0].colorWriteMask == 15 && state.pMultisampleState->rasterizationSamples == VK_SAMPLE_COUNT_1_BIT, "Native graphics output masks or samples were changed");
    Require(!state.pDepthStencilState->depthTestEnable && !state.pDepthStencilState->stencilTestEnable, "Native graphics enabled unused depth tests");
    source.depth.stencilEnable = true;
    source.depth.front = {1, 2, 3, 4};
    source.depth.back = {5, 6, 7, 8};
    description = MakeSerializedFixedDescription(source);
    Require(description.state.DepthStencilState.BackFace.StencilPassOp == D3D12_STENCIL_OP_REPLACE, "Native shared stencil face was not applied to back faces");
    source.depth.separateBackFace = true;
    description = MakeSerializedFixedDescription(source);
    Require(description.state.DepthStencilState.BackFace.StencilPassOp == D3D12_STENCIL_OP_INCR, "Native separate stencil face was lost");
    auto invalid = source;
    invalid.depth.backReadMask ^= 1;
    Reject<std::runtime_error>([&] { (void)MakeSerializedFixedDescription(invalid); });
    invalid = source;
    invalid.depth.reference = 1;
    Reject<std::runtime_error>([&] { (void)MakeSerializedFixedDescription(invalid); });
    invalid = source;
    invalid.depth.front.fail = 9;
    Reject<std::runtime_error>([&] { (void)MakeSerializedFixedDescription(invalid); });
    invalid = source;
    invalid.depth.boundsEnable = true;
    Reject<std::runtime_error>([&] { (void)MakeSerializedFixedDescription(invalid); });
    invalid = source;
    invalid.alphaToCoverage = 2;
    Reject<std::runtime_error>([&] { (void)MakeSerializedFixedDescription(invalid); });
}
