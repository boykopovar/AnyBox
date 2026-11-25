#include "State.hpp"

namespace {
VkCompareOp Compare(D3D12_COMPARISON_FUNC value) {
    if (value < D3D12_COMPARISON_FUNC_NEVER || value > D3D12_COMPARISON_FUNC_ALWAYS) throw std::invalid_argument("Offline graphics comparison function is invalid");
    return static_cast<VkCompareOp>(value - 1);
}

VkStencilOp Stencil(D3D12_STENCIL_OP value) {
    constexpr std::array values{VK_STENCIL_OP_KEEP, VK_STENCIL_OP_ZERO, VK_STENCIL_OP_REPLACE, VK_STENCIL_OP_INCREMENT_AND_CLAMP, VK_STENCIL_OP_DECREMENT_AND_CLAMP, VK_STENCIL_OP_INVERT, VK_STENCIL_OP_INCREMENT_AND_WRAP, VK_STENCIL_OP_DECREMENT_AND_WRAP};
    if (value < D3D12_STENCIL_OP_KEEP || value > D3D12_STENCIL_OP_DECR) throw std::invalid_argument("Offline graphics stencil operation is invalid");
    return values[value - 1];
}

VkStencilOpState Face(const D3D12_DEPTH_STENCILOP_DESC& source, std::uint8_t readMask, std::uint8_t writeMask) {
    return {Stencil(source.StencilFailOp), Stencil(source.StencilPassOp), Stencil(source.StencilDepthFailOp), Compare(source.StencilFunc), readMask, writeMask, 0};
}
}

void AnyBox::Graphics::GraphicsFixedState::initializeDepth(const D3D12_DEPTH_STENCIL_DESC& source, BOOL bounds) {
    if (bounds && !context.GetEnabledFeatures().depthBounds) throw std::runtime_error("Offline graphics depth bounds testing is unsupported by the device");
    if (source.DepthWriteMask != D3D12_DEPTH_WRITE_MASK_ZERO && source.DepthWriteMask != D3D12_DEPTH_WRITE_MASK_ALL) throw std::invalid_argument("Offline graphics depth write mask is invalid");
    depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth.depthTestEnable = source.DepthEnable != FALSE;
    depth.depthWriteEnable = source.DepthEnable && source.DepthWriteMask == D3D12_DEPTH_WRITE_MASK_ALL;
    depth.depthCompareOp = source.DepthEnable ? Compare(source.DepthFunc) : VK_COMPARE_OP_ALWAYS;
    depth.depthBoundsTestEnable = bounds != FALSE;
    depth.minDepthBounds = 0;
    depth.maxDepthBounds = 1;
    depth.stencilTestEnable = source.StencilEnable != FALSE;
    if (source.StencilEnable) {
        depth.front = Face(source.FrontFace, source.StencilReadMask, source.StencilWriteMask);
        depth.back = Face(source.BackFace, source.StencilReadMask, source.StencilWriteMask);
    }
}
